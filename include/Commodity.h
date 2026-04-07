#ifndef COMMODITY_H
#define COMMODITY_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <sqlite3.h>

class Commodity
{
public:
    Commodity(int id, const std::string &name, const std::string &type,
              const std::string &merchant, const std::string &description,
              double basePrice, int storage, double discount = 1.0)
        : _id(id), _name(name), _type(type), _merchant(merchant),
          _description(description), _basePrice(basePrice),
          _discount(discount), _storage(storage) {}

    int getId() const { return _id; }
    std::string getName() const { return _name; }
    std::string getType() const { return _type; }
    std::string getDescription() const { return _description; }
    std::string getMerchant() const { return _merchant; }
    double getBasePrice() const { return _basePrice; }
    double getPrice() const { return _basePrice * _discount; }
    double getDiscount() const { return _discount; }
    int getStorage() const { return _storage; }

    void setBasePrice(double price) { _basePrice = price; }
    void setDiscount(double discount) { _discount = discount; }
    void setStorage(int storage) { _storage = storage; }

private:
    int _id;
    std::string _name;
    std::string _type;
    std::string _merchant;
    std::string _description;
    double _basePrice;
    double _discount;
    int _storage;
};

// 商品管理类（按需缓存：查缓存未命中时从 DB 加载）
class CommodityManager
{
public:
    explicit CommodityManager(sqlite3 *db);
    ~CommodityManager() = default;
    CommodityManager(const CommodityManager &) = delete;
    CommodityManager &operator=(const CommodityManager &) = delete;

    bool addCommodity(const std::string &type, const std::string &name,
                      const std::string &merchant, const std::string &description = "",
                      double price = 10.0, int storage = 0, double discount = 1.0);

    // 搜索商品（查 DB，结果加入缓存）
    std::vector<const Commodity *> findCommodity(const std::string &name = "");
    // 获取特定商家的所有商品（查 DB，结果加入缓存）
    std::vector<const Commodity *> getCommodityByMerchant(const std::string &merchantName);
    // 按名称查找（缓存优先）
    Commodity *getCommodityByName(const std::string &name);
    // 按 ID 查找（缓存优先）
    Commodity *getCommodityById(int id);

    bool updatePrice(Commodity *commodity, double newPrice);
    bool updateStock(Commodity *commodity, int newStock);
    bool updateDiscount(Commodity *commodity, double newDiscount);
    // 返回受影响的行数，-1 表示参数错误
    int batchUpdateDiscount(const std::string &merchant, const std::string &type, double newDiscount);

private:
    sqlite3 *_db;
    std::mutex _cacheMutex;
    std::unordered_map<std::string, std::unique_ptr<Commodity>> _nameMap;
    std::unordered_map<int, Commodity *> _idMap;

    // 缓存操作（须持有 _cacheMutex）
    Commodity *cacheCommodity(std::unique_ptr<Commodity> c);
    // 按 name 从 DB 加载
    Commodity *loadByName(const std::string &name);
    // 按 id 从 DB 加载
    Commodity *loadById(int id);
};

#endif
