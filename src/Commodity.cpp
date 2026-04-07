#include <iostream>
#include <sqlite3.h>
#include "Commodity.h"

CommodityManager::CommodityManager(sqlite3 *db) : _db(db) {}

Commodity *CommodityManager::cacheCommodity(std::unique_ptr<Commodity> c)
{
    std::string name = c->getName();
    int id = c->getId();
    // 双重检查：可能已被其他线程缓存
    auto &slot = _nameMap[name];
    if (slot)
    {
        // 已存在，丢弃新加载的
        return slot.get();
    }
    auto *ptr = c.get();
    _idMap[id] = ptr;
    slot = std::move(c);
    return ptr;
}

Commodity *CommodityManager::loadByName(const std::string &name)
{
    const char *sql = "SELECT id, type, merchant, description, base_price, storage, discount FROM commodities WHERE name = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return nullptr;
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    std::unique_ptr<Commodity> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result = std::make_unique<Commodity>(
            sqlite3_column_int(stmt, 0),
            name,
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)),
            sqlite3_column_double(stmt, 4),
            sqlite3_column_int(stmt, 5),
            sqlite3_column_double(stmt, 6));
    }
    sqlite3_finalize(stmt);

    if (!result) return nullptr;

    std::lock_guard<std::mutex> lock(_cacheMutex);
    return cacheCommodity(std::move(result));
}

Commodity *CommodityManager::loadById(int id)
{
    const char *sql = "SELECT name, type, merchant, description, base_price, storage, discount FROM commodities WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return nullptr;
    sqlite3_bind_int(stmt, 1, id);

    std::unique_ptr<Commodity> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string name(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
        result = std::make_unique<Commodity>(
            id, name,
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)),
            sqlite3_column_double(stmt, 4),
            sqlite3_column_int(stmt, 5),
            sqlite3_column_double(stmt, 6));
    }
    sqlite3_finalize(stmt);

    if (!result) return nullptr;

    std::lock_guard<std::mutex> lock(_cacheMutex);
    return cacheCommodity(std::move(result));
}

bool CommodityManager::addCommodity(const std::string &type, const std::string &name,
                                     const std::string &merchant, const std::string &description,
                                     double price, int storage, double discount)
{
    if (type != "Food" && type != "Clothes" && type != "Book" && type != "Electronics")
    {
        std::cerr << "未知类型：" << type << '\n';
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        if (_nameMap.count(name))
        {
            std::cerr << name << " 已经存在" << '\n';
            return false;
        }
    }

    const char *sql = "INSERT INTO commodities (name, type, merchant, description, base_price, storage, discount) VALUES (?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "SQL prepare failed: " << sqlite3_errmsg(_db) << '\n';
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, merchant.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, price);
    sqlite3_bind_int(stmt, 6, storage);
    sqlite3_bind_double(stmt, 7, discount);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "Insert commodity failed: " << sqlite3_errmsg(_db) << '\n';
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);

    int id = static_cast<int>(sqlite3_last_insert_rowid(_db));
    auto c = std::make_unique<Commodity>(id, name, type, merchant, description, price, storage, discount);
    std::lock_guard<std::mutex> lock(_cacheMutex);
    _idMap[id] = c.get();
    _nameMap[name] = std::move(c);
    return true;
}

std::vector<const Commodity *> CommodityManager::findCommodity(const std::string &name)
{
    std::vector<const Commodity *> results;

    std::string sql = name.empty()
        ? "SELECT id, name, type, merchant, description, base_price, storage, discount FROM commodities"
        : "SELECT id, name, type, merchant, description, base_price, storage, discount FROM commodities WHERE name LIKE '%' || ? || '%'";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return results;

    if (!name.empty())
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    std::lock_guard<std::mutex> lock(_cacheMutex);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        auto c = std::make_unique<Commodity>(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)),
            sqlite3_column_double(stmt, 5),
            sqlite3_column_int(stmt, 6),
            sqlite3_column_double(stmt, 7));

        results.push_back(cacheCommodity(std::move(c)));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<const Commodity *> CommodityManager::getCommodityByMerchant(const std::string &merchantName)
{
    std::vector<const Commodity *> results;

    const char *sql = "SELECT id, name, type, merchant, description, base_price, storage, discount FROM commodities WHERE merchant = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return results;

    sqlite3_bind_text(stmt, 1, merchantName.c_str(), -1, SQLITE_TRANSIENT);

    std::lock_guard<std::mutex> lock(_cacheMutex);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        auto c = std::make_unique<Commodity>(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)),
            sqlite3_column_double(stmt, 5),
            sqlite3_column_int(stmt, 6),
            sqlite3_column_double(stmt, 7));

        results.push_back(cacheCommodity(std::move(c)));
    }
    sqlite3_finalize(stmt);
    return results;
}

Commodity *CommodityManager::getCommodityByName(const std::string &name)
{
    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        auto it = _nameMap.find(name);
        if (it != _nameMap.end())
            return it->second.get();
    }
    return loadByName(name);
}

Commodity *CommodityManager::getCommodityById(int id)
{
    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        auto it = _idMap.find(id);
        if (it != _idMap.end())
            return it->second;
    }
    return loadById(id);
}

bool CommodityManager::updatePrice(Commodity *commodity, double newPrice)
{
    if (newPrice < 0) return false;

    const char *sql = "UPDATE commodities SET base_price = ? WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_double(stmt, 1, newPrice);
    sqlite3_bind_int(stmt, 2, commodity->getId());
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    if (ok) commodity->setBasePrice(newPrice);
    return ok;
}

bool CommodityManager::updateStock(Commodity *commodity, int newStock)
{
    if (newStock < 0) return false;

    const char *sql = "UPDATE commodities SET storage = ? WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, newStock);
    sqlite3_bind_int(stmt, 2, commodity->getId());
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    if (ok) commodity->setStorage(newStock);
    return ok;
}

bool CommodityManager::updateDiscount(Commodity *commodity, double newDiscount)
{
    if (newDiscount < 0 || newDiscount > 1) return false;

    const char *sql = "UPDATE commodities SET discount = ? WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_double(stmt, 1, newDiscount);
    sqlite3_bind_int(stmt, 2, commodity->getId());
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    if (ok) commodity->setDiscount(newDiscount);
    return ok;
}

int CommodityManager::batchUpdateDiscount(const std::string &merchant, const std::string &type, double newDiscount)
{
    if (newDiscount < 0 || newDiscount > 1) return -1;

    const char *sql = "UPDATE commodities SET discount = ? WHERE merchant = ? AND type = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_double(stmt, 1, newDiscount);
    sqlite3_bind_text(stmt, 2, merchant.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!ok) return -1;

    int count = sqlite3_changes(_db);

    // 同步缓存
    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        for (auto &[n, c] : _nameMap)
        {
            if (c->getMerchant() == merchant && c->getType() == type)
                c->setDiscount(newDiscount);
        }
    }
    return count;
}
