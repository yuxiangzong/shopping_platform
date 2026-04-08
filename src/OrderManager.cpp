#include <iostream>
#include <sqlite3.h>
#include "OrderManager.h"
#include "User.h"
#include "Commodity.h"

OrderManager::OrderManager(sqlite3 *db, UserManager &userManager, CommodityManager &commodityManager)
    : _db(db), _userManager(userManager), _commodityManager(commodityManager) {}

// ---- 购物车操作 ----

std::vector<OrderItem> OrderManager::getCartItems(int userId)
{
    std::vector<OrderItem> items;
    const char *sql = R"(
        SELECT c.name, ci.quantity, c.base_price * c.discount
        FROM cart_items ci
        JOIN commodities c ON ci.commodity_id = c.id
        WHERE ci.user_id = ?
    )";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return items;

    sqlite3_bind_int(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        items.push_back({
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), // name
            sqlite3_column_int(stmt, 1),                                    // quantity
            sqlite3_column_double(stmt, 2)                                  // price
        });
    }
    sqlite3_finalize(stmt);
    return items;
}

bool OrderManager::addToCart(int userId, const std::string &commodityName, int quantity)
{
    if (quantity <= 0)
    {
        std::cerr << "数量必须大于0" << '\n';
        return false;
    }

    Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
    if (!commodity)
    {
        std::cerr << "商品不存在" << '\n';
        return false;
    }

    // 检查库存（考虑购物车中已有的数量）
    const char *checkSql = "SELECT quantity FROM cart_items WHERE user_id = ? AND commodity_id = ?";
    sqlite3_stmt *checkStmt;
    int existingQty = 0;
    if (sqlite3_prepare_v2(_db, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(checkStmt, 1, userId);
        sqlite3_bind_int(checkStmt, 2, commodity->getId());
        if (sqlite3_step(checkStmt) == SQLITE_ROW)
            existingQty = sqlite3_column_int(checkStmt, 0);
        sqlite3_finalize(checkStmt);
    }

    if (existingQty + quantity > commodity->getStorage())
    {
        std::cerr << "库存不足 (库存: " << commodity->getStorage()
                  << ", 已在购物车: " << existingQty
                  << ", 请求数量: " << quantity << ")" << '\n';
        return false;
    }

    // INSERT OR REPLACE
    const char *sql = "INSERT INTO cart_items (user_id, commodity_id, quantity) VALUES (?, ?, ?) "
                      "ON CONFLICT(user_id, commodity_id) DO UPDATE SET quantity = quantity + ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "SQL prepare failed: " << sqlite3_errmsg(_db) << '\n';
        return false;
    }
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, commodity->getId());
    sqlite3_bind_int(stmt, 3, quantity);
    sqlite3_bind_int(stmt, 4, quantity);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool OrderManager::removeFromCart(int userId, const std::string &commodityName)
{
    Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
    if (!commodity) return false;

    const char *sql = "DELETE FROM cart_items WHERE user_id = ? AND commodity_id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, commodity->getId());
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool OrderManager::updateCartQuantity(int userId, const std::string &commodityName, int newQuantity)
{
    if (newQuantity <= 0) return false;

    Commodity *commodity = _commodityManager.getCommodityByName(commodityName);
    if (!commodity) return false;

    if (newQuantity > commodity->getStorage())
    {
        std::cerr << "库存不足" << '\n';
        return false;
    }

    const char *sql = "UPDATE cart_items SET quantity = ? WHERE user_id = ? AND commodity_id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, newQuantity);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_int(stmt, 3, commodity->getId());
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

int OrderManager::checkout(int userId, bool immediatePayment, std::string &errorMsg)
{
    // 读取购物车
    auto cartItems = getCartItems(userId);
    if (cartItems.empty())
    {
        errorMsg = "购物车为空";
        return -1;
    }

    // 验证库存充足
    for (const auto &item : cartItems)
    {
        Commodity *c = _commodityManager.getCommodityByName(item.commodityName);
        if (!c || c->getStorage() < item.quantity)
        {
            errorMsg = "商品 " + item.commodityName + " 库存不足";
            return -1;
        }
    }

    // 开始事务
    if (sqlite3_exec(_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        errorMsg = "事务启动失败";
        return -1;
    }

    // 插入订单
    const char *orderSql = "INSERT INTO orders (user_id) VALUES (?)";
    sqlite3_stmt *orderStmt;
    if (sqlite3_prepare_v2(_db, orderSql, -1, &orderStmt, nullptr) != SQLITE_OK)
    {
        sqlite3_exec(_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "创建订单失败";
        return -1;
    }
    sqlite3_bind_int(orderStmt, 1, userId);
    if (sqlite3_step(orderStmt) != SQLITE_DONE)
    {
        sqlite3_finalize(orderStmt);
        sqlite3_exec(_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "创建订单失败";
        return -1;
    }
    sqlite3_finalize(orderStmt);
    int orderId = static_cast<int>(sqlite3_last_insert_rowid(_db));

    // 插入订单项 & 扣减库存
    const char *itemSql = "INSERT INTO order_items (order_id, commodity_id, quantity, unit_price) VALUES (?, ?, ?, ?)";
    const char *stockSql = "UPDATE commodities SET storage = storage - ? WHERE id = ? AND storage >= ?";
    sqlite3_stmt *itemStmt, *stockStmt;
    sqlite3_prepare_v2(_db, itemSql, -1, &itemStmt, nullptr);
    sqlite3_prepare_v2(_db, stockSql, -1, &stockStmt, nullptr);

    for (const auto &item : cartItems)
    {
        Commodity *c = _commodityManager.getCommodityByName(item.commodityName);

        // 订单项
        sqlite3_bind_int(itemStmt, 1, orderId);
        sqlite3_bind_int(itemStmt, 2, c->getId());
        sqlite3_bind_int(itemStmt, 3, item.quantity);
        sqlite3_bind_double(itemStmt, 4, item.unitPrice);
        if (sqlite3_step(itemStmt) != SQLITE_DONE)
        {
            sqlite3_finalize(itemStmt);
            sqlite3_finalize(stockStmt);
            sqlite3_exec(_db, "ROLLBACK;", nullptr, nullptr, nullptr);
            errorMsg = "插入订单项失败";
            return -1;
        }
        sqlite3_reset(itemStmt);

        // 扣库存
        sqlite3_bind_int(stockStmt, 1, item.quantity);
        sqlite3_bind_int(stockStmt, 2, c->getId());
        sqlite3_bind_int(stockStmt, 3, item.quantity);
        if (sqlite3_step(stockStmt) != SQLITE_DONE)
        {
            sqlite3_finalize(itemStmt);
            sqlite3_finalize(stockStmt);
            sqlite3_exec(_db, "ROLLBACK;", nullptr, nullptr, nullptr);
            errorMsg = "扣减库存失败";
            return -1;
        }
        sqlite3_reset(stockStmt);
    }
    sqlite3_finalize(itemStmt);
    sqlite3_finalize(stockStmt);

    // 清空购物车
    const char *clearSql = "DELETE FROM cart_items WHERE user_id = ?";
    sqlite3_stmt *clearStmt;
    sqlite3_prepare_v2(_db, clearSql, -1, &clearStmt, nullptr);
    sqlite3_bind_int(clearStmt, 1, userId);
    sqlite3_step(clearStmt);
    sqlite3_finalize(clearStmt);

    // 同步缓存中的库存
    for (const auto &item : cartItems)
    {
        Commodity *c = _commodityManager.getCommodityByName(item.commodityName);
        if (c)
            c->setStorage(c->getStorage() - item.quantity);
    }

    // 如果立即支付
    if (immediatePayment)
    {
        User *consumer = _userManager.getUserById(userId);
        if (!consumer)
        {
            sqlite3_exec(_db, "ROLLBACK;", nullptr, nullptr, nullptr);
            errorMsg = "用户不存在";
            return -1;
        }

        // 计算总金额
        double total = 0.0;
        for (const auto &item : cartItems)
            total += item.unitPrice * item.quantity;

        if (!consumer->subtractBalance(total))
        {
            sqlite3_exec(_db, "ROLLBACK;", nullptr, nullptr, nullptr);
            errorMsg = "余额不足";
            return -1;
        }

        // 分配给商家
        for (const auto &item : cartItems)
        {
            Commodity *c = _commodityManager.getCommodityByName(item.commodityName);
            User *merchant = _userManager.getUserByUsername(c->getMerchant());
            if (merchant)
            {
                merchant->addBalance(item.unitPrice * item.quantity);
                _userManager.updateBalance(merchant);
            }
        }

        // 更新订单状态
        const char *paySql = "UPDATE orders SET status = 'PAID' WHERE id = ?";
        sqlite3_stmt *payStmt;
        sqlite3_prepare_v2(_db, paySql, -1, &payStmt, nullptr);
        sqlite3_bind_int(payStmt, 1, orderId);
        sqlite3_step(payStmt);
        sqlite3_finalize(payStmt);

        _userManager.updateBalance(consumer);
    }

    // 提交事务
    if (sqlite3_exec(_db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        errorMsg = "事务提交失败";
        return -1;
    }

    return orderId;
}

// ---- 订单操作 ----

std::vector<Order> OrderManager::getOrders(int userId)
{
    std::vector<Order> orders;
    const char *sql = "SELECT id, status, created_at FROM orders WHERE user_id = ? ORDER BY id DESC";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return orders;

    sqlite3_bind_int(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int orderId = sqlite3_column_int(stmt, 0);
        std::string statusStr(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        std::string createdAt(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));

        Order::Status status = (statusStr == "PAID") ? Order::PAID
                               : (statusStr == "CANCELLED") ? Order::CANCELLED
                                                            : Order::PENDING;

        // 获取订单项
        std::vector<OrderItem> items;
        const char *itemSql = R"(
            SELECT c.name, oi.quantity, oi.unit_price
            FROM order_items oi
            JOIN commodities c ON oi.commodity_id = c.id
            WHERE oi.order_id = ?
        )";
        sqlite3_stmt *itemStmt;
        if (sqlite3_prepare_v2(_db, itemSql, -1, &itemStmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(itemStmt, 1, orderId);
            while (sqlite3_step(itemStmt) == SQLITE_ROW)
            {
                items.push_back({
                    reinterpret_cast<const char *>(sqlite3_column_text(itemStmt, 0)),
                    sqlite3_column_int(itemStmt, 1),
                    sqlite3_column_double(itemStmt, 2)});
            }
            sqlite3_finalize(itemStmt);
        }

        orders.emplace_back(orderId, userId, status, createdAt, std::move(items));
    }
    sqlite3_finalize(stmt);
    return orders;
}

bool OrderManager::payOrder(int userId, int orderId, std::string &errorMsg)
{
    // 获取订单信息
    const char *sql = "SELECT status FROM orders WHERE id = ? AND user_id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        errorMsg = "SQL 错误";
        return false;
    }
    sqlite3_bind_int(stmt, 1, orderId);
    sqlite3_bind_int(stmt, 2, userId);

    std::string statusStr;
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        errorMsg = "订单不存在";
        return false;
    }
    statusStr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);

    if (statusStr != "PENDING")
    {
        errorMsg = "订单状态错误，无法支付";
        return false;
    }

    // 获取订单项
    const char *itemSql = R"(
        SELECT c.name, oi.quantity, oi.unit_price, c.merchant
        FROM order_items oi
        JOIN commodities c ON oi.commodity_id = c.id
        WHERE oi.order_id = ?
    )";
    sqlite3_stmt *itemStmt;
    if (sqlite3_prepare_v2(_db, itemSql, -1, &itemStmt, nullptr) != SQLITE_OK)
    {
        errorMsg = "SQL 错误";
        return false;
    }
    sqlite3_bind_int(itemStmt, 1, orderId);

    // 计算总金额，按商家分组
    double total = 0.0;
    std::unordered_map<std::string, double> merchantAmounts;
    while (sqlite3_step(itemStmt) == SQLITE_ROW)
    {
        int qty = sqlite3_column_int(itemStmt, 1);
        double price = sqlite3_column_double(itemStmt, 2);
        std::string merchant(reinterpret_cast<const char *>(sqlite3_column_text(itemStmt, 3)));
        double amount = price * qty;
        total += amount;
        merchantAmounts[merchant] += amount;
    }
    sqlite3_finalize(itemStmt);

    // 扣消费者余额
    User *consumer = _userManager.getUserById(userId);
    if (!consumer)
    {
        errorMsg = "用户不存在";
        return false;
    }

    if (!consumer->subtractBalance(total))
    {
        errorMsg = "余额不足，当前余额：" + std::to_string(consumer->getBalance());
        return false;
    }

    // 开始事务
    sqlite3_exec(_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    // 给商家转账
    for (const auto &[merchantName, amount] : merchantAmounts)
    {
        User *merchant = _userManager.getUserByUsername(merchantName);
        if (merchant)
        {
            merchant->addBalance(amount);
            _userManager.updateBalance(merchant);
        }
    }

    // 更新订单状态
    const char *updateSql = "UPDATE orders SET status = 'PAID' WHERE id = ?";
    sqlite3_stmt *updateStmt;
    sqlite3_prepare_v2(_db, updateSql, -1, &updateStmt, nullptr);
    sqlite3_bind_int(updateStmt, 1, orderId);
    sqlite3_step(updateStmt);
    sqlite3_finalize(updateStmt);

    _userManager.updateBalance(consumer);

    sqlite3_exec(_db, "COMMIT;", nullptr, nullptr, nullptr);
    return true;
}

bool OrderManager::cancelOrderInternal(int orderId)
{
    // 恢复库存
    const char *itemSql = "SELECT commodity_id, quantity FROM order_items WHERE order_id = ?";
    const char *restoreSql = "UPDATE commodities SET storage = storage + ? WHERE id = ?";
    const char *updateSql = "UPDATE orders SET status = 'CANCELLED' WHERE id = ?";

    sqlite3_stmt *itemStmt = nullptr, *restoreStmt = nullptr, *updateStmt = nullptr;
    if (sqlite3_prepare_v2(_db, itemSql, -1, &itemStmt, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(_db, restoreSql, -1, &restoreStmt, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(_db, updateSql, -1, &updateStmt, nullptr) != SQLITE_OK)
    {
        sqlite3_finalize(itemStmt);
        sqlite3_finalize(restoreStmt);
        sqlite3_finalize(updateStmt);
        return false;
    }

    // 恢复库存
    sqlite3_bind_int(itemStmt, 1, orderId);
    while (sqlite3_step(itemStmt) == SQLITE_ROW)
    {
        int commodityId = sqlite3_column_int(itemStmt, 0);
        int quantity = sqlite3_column_int(itemStmt, 1);

        sqlite3_bind_int(restoreStmt, 1, quantity);
        sqlite3_bind_int(restoreStmt, 2, commodityId);
        sqlite3_step(restoreStmt);
        sqlite3_reset(restoreStmt);

        // 同步缓存
        Commodity *c = _commodityManager.getCommodityById(commodityId);
        if (c)
            c->setStorage(c->getStorage() + quantity);
    }
    sqlite3_finalize(itemStmt);
    sqlite3_finalize(restoreStmt);

    // 更新订单状态
    sqlite3_bind_int(updateStmt, 1, orderId);
    sqlite3_step(updateStmt);
    sqlite3_finalize(updateStmt);

    return true;
}

int OrderManager::cancelExpiredOrders(int timeoutMinutes)
{
    // 查找所有超时的 PENDING 订单
    const char *findSql = "SELECT id FROM orders "
                          "WHERE status = 'PENDING' "
                          "AND created_at < datetime('now', '-' || ? || ' minutes')";
    sqlite3_stmt *findStmt;
    if (sqlite3_prepare_v2(_db, findSql, -1, &findStmt, nullptr) != SQLITE_OK)
        return 0;

    sqlite3_bind_int(findStmt, 1, timeoutMinutes);

    std::vector<int> expiredIds;
    while (sqlite3_step(findStmt) == SQLITE_ROW)
        expiredIds.push_back(sqlite3_column_int(findStmt, 0));
    sqlite3_finalize(findStmt);

    if (expiredIds.empty())
        return 0;

    sqlite3_exec(_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    for (int orderId : expiredIds)
    {
        if (cancelOrderInternal(orderId))
            std::cout << "订单 " << orderId << " 已自动取消（超时未支付）" << '\n';
    }

    sqlite3_exec(_db, "COMMIT;", nullptr, nullptr, nullptr);
    return static_cast<int>(expiredIds.size());
}

bool OrderManager::cancelOrder(int userId, int orderId, std::string &errorMsg)
{
    // 获取订单状态
    const char *sql = "SELECT status FROM orders WHERE id = ? AND user_id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        errorMsg = "SQL 错误";
        return false;
    }
    sqlite3_bind_int(stmt, 1, orderId);
    sqlite3_bind_int(stmt, 2, userId);

    std::string statusStr;
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        errorMsg = "订单不存在";
        return false;
    }
    statusStr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);

    if (statusStr != "PENDING")
    {
        errorMsg = "订单状态错误，无法取消";
        return false;
    }

    sqlite3_exec(_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    cancelOrderInternal(orderId);
    sqlite3_exec(_db, "COMMIT;", nullptr, nullptr, nullptr);
    return true;
}
