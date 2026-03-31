#ifndef COMMODITY_H
#define COMMODITY_H

#include <string>
#include <vector>

// 商品基类，抽象类
class Commodity
{
public:
    Commodity(const std::string &name, const std::string &merchant, const std::string &description = "", double basePrice = 10.0, int storage = 0, double discount = 1.0);
    virtual ~Commodity() = default;

    std::string getName() const;                      // 获取商品名
    std::string getDescription() const;               // 获取商品描述
    std::string getMerchant() const;                  // 获取商家名
    double getBasePrice() const;                      // 获取商品原价
    virtual double getPrice() const;                  // 获取商品价格（原价*折扣）
    double getDiscount() const;                       // 获取商品折扣
    int getStorage() const;                           // 获取商品库存
    virtual std::string getCommodityType() const = 0; // 获取商品类型

    bool setBasePrice(double price);   // 设置商品原价
    bool setDiscount(double discount); // 设置商品折扣（0~1）
    bool setStorage(int storage);      // 设置商品库存
    virtual void print() const;        // 打印商品信息
protected:
    std::string _name;        // 商品名
    std::string _description; // 商品描述
    std::string _merchant;    // 商家名
    double _basePrice;        // 商品原价
    double _discount;         // 商品折扣（0~1,1为原价）
    int _storage;             // 商品库存
};

// 食物类，继承自商品基类
class Food : public Commodity
{
public:
    Food(const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0)
        : Commodity(name, merchant, description, price, storage, discount) {}

    std::string getCommodityType() const override;
};

// 服装类，继承自商品类基类
class Clothes : public Commodity
{
public:
    Clothes(const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0)
        : Commodity(name, merchant, description, price, storage, discount) {}

    std::string getCommodityType() const override;
};

// 图书类，继承自商品类基类
class Book : public Commodity
{
public:
    Book(const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0)
        : Commodity(name, merchant, description, price, storage, discount) {}

    std::string getCommodityType() const override;
};

// 电子商品类，继承自商品基类
class Electronics : public Commodity
{
public:
    Electronics(const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0)
        : Commodity(name, merchant, description, price, storage, discount) {}

    std::string getCommodityType() const override;
};

// 商品管理类，用于管理不同类型的商品
class CommodityManager
{
public:
    CommodityManager();
    ~CommodityManager();
    bool loadCommodities();                                                                                                                                                                             // 加载商品数据
    bool saveCommodities() const;                                                                                                                                                                       // 保存商品数据
    bool addCommodity(const std::string &type, const std::string &name, const std::string &merchant, const std::string &description = "", double price = 10.0, int storage = 0, double discount = 1.0); // 添加商品

    void showCommodities(std::vector<Commodity *> commodities) const;           // 显示商品列表
    void manageCommodity(const std::string &merchant);                          // 管理特定商家的商品
    std::vector<Commodity *> findCommodity(const std::string &name = "") const; // 搜索商品
private:
    std::vector<Commodity *> _commodities;        // 商品列表
    std::string _filename = "./commodities.json"; // 商品数据存储文件名
};

#endif