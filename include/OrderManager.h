#ifndef ORDERMANAGER_H
#define ORDERMANAGER_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include "Order.h"
#include "User.h"

class UserManager;
class CommodityManager;

// 订单与购物车管理类
class OrderManager
{
public:
    OrderManager(sqlite3 *db, UserManager &userManager, CommodityManager &commodityManager);

    // ---- 购物车操作 ----
    // 查看购物车
    std::vector<OrderItem> getCartItems(int userId);
    // 添加商品到购物车
    bool addToCart(int userId, const std::string &commodityName, int quantity);
    // 从购物车移除商品
    bool removeFromCart(int userId, const std::string &commodityName);
    // 修改购物车商品数量
    bool updateCartQuantity(int userId, const std::string &commodityName, int newQuantity);
    // 结算购物车 -> 创建订单，immediatePayment=true 则直接支付
    // 返回订单 ID（失败返回 -1）
    int checkout(int userId, bool immediatePayment, std::string &errorMsg);

    // ---- 订单操作 ----
    // 获取用户所有订单
    std::vector<Order> getOrders(int userId);
    // 支付订单
    bool payOrder(int userId, int orderId, std::string &errorMsg);
    // 取消订单
    bool cancelOrder(int userId, int orderId, std::string &errorMsg);
    // 自动取消超时未支付的订单（由后台定时线程调用）
    int cancelExpiredOrders(int timeoutMinutes = 30);

private:
    sqlite3 *_db;
    UserManager &_userManager;
    CommodityManager &_commodityManager;

    // 内部取消订单（恢复库存 + 更新状态），调用者需持有 _dataMutex 写锁
    bool cancelOrderInternal(int orderId);
};

#endif
