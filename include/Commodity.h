#ifndef COMMODITY_H
#define COMMODITY_H

#include <string>
#include <vector>

// 商品基类，抽象类
class Commodity
{
public:
    Commodity(const std::string &name, const std::string &merchant, const std::string &description = "", double basePrice = 10.0, int storage = 0, double discount = 1.0)
        : _name(name), _description(description), _merchant(merchant), _basePrice(basePrice), _discount(discount), _storage(storage) {}
    virtual ~Commodity() = default;

    std::string getName() const { return _name; }
    std::string getDescription() const { return _description; }
    std::string getMerchant() const { return _merchant; }
    double getBasePrice() const { return _basePrice; }
    virtual double getPrice() const { return _basePrice * _discount; }
    double getDiscount() const { return _discount; }
    int getStorage() const { return _storage; }
    virtual std::string getCommodityType() const = 0;

    bool setBasePrice(double price);
    bool setDiscount(double discount);
    bool setStorage(int storage);
    virtual void print() const;

protected:
    std::string _name;
    std::string _description;
    std::string _merchant;
    double _basePrice;
    double _discount;
    int _storage;
};

// 食物类，继承自商品基类
class Food : public Commodity
{
public:
    Food(const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0)
        : Commodity(name, merchant, description, price, storage, discount) {}

    std::string getCommodityType() const override { return "Food"; }
};

// 服装类，继承自商品类基类
class Clothes : public Commodity
{
public:
    Clothes(const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0)
        : Commodity(name, merchant, description, price, storage, discount) {}

    std::string getCommodityType() const override { return "Clothes"; }
};

// 图书类，继承自商品类基类
class Book : public Commodity
{
public:
    Book(const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0)
        : Commodity(name, merchant, description, price, storage, discount) {}

    std::string getCommodityType() const override { return "Book"; }
};

// 电子商品类，继承自商品基类
class Electronics : public Commodity
{
public:
    Electronics(const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0)
        : Commodity(name, merchant, description, price, storage, discount) {}

    std::string getCommodityType() const override { return "Electronics"; }
};

// 商品管理类，用于管理不同类型的商品
class CommodityManager
{
public:
    CommodityManager();
    ~CommodityManager();

    bool loadCommodities();
    bool saveCommodities() const;
    bool addCommodity(const std::string &type, const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0);
    void showCommodities(std::vector<Commodity *> commodities) const;
    void manageCommodity(const std::string &merchant);
    std::vector<Commodity *> findCommodity(const std::string &name = "") const;
    std::vector<Commodity *> getCommodityByMerchant(const std::string &merchantName) const;

private:
    std::vector<Commodity *> _commodities;
    std::string _filename = "./commodities.json";
};

#endif
