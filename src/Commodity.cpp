#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "Commodity.h"

// ==================== Commodity ====================

bool Commodity::setBasePrice(double price)
{
    if (price < 0)
    {
        std::cerr << "价格不能为负数" << '\n';
        return false;
    }
    _basePrice = price;
    return true;
}

bool Commodity::setDiscount(double discount)
{
    if (discount < 0 || discount > 1)
    {
        std::cerr << "折扣必须在0到1之间" << '\n';
        return false;
    }
    _discount = discount;
    return true;
}

bool Commodity::setStorage(int storage)
{
    if (storage < 0)
    {
        std::cerr << "库存不能为负数" << '\n';
        return false;
    }
    _storage = storage;
    return true;
}

void Commodity::print() const
{
    const std::string separator(50, '-');
    std::cout << separator << '\n';
    std::cout << "商品类型: " << getCommodityType() << '\n';
    std::cout << "名称: " << getName() << '\n';
    std::cout << "描述: " << getDescription() << '\n';
    std::cout << "商家: " << getMerchant() << '\n';
    std::cout << "现价: " << getPrice() << '\n';
    std::cout << "基础价格: " << getBasePrice() << '\n';
    std::cout << "折扣: " << (getDiscount() * 100) << "%\n";
    std::cout << "库存: " << getStorage() << "件\n";
}

// ==================== CommodityManager ====================

CommodityManager::CommodityManager() { loadCommodities(); }
CommodityManager::~CommodityManager()
{
    saveCommodities();
    std::cout << "商品信息已保存" << '\n';
    // unique_ptr 自动释放商品对象
}

bool CommodityManager::loadCommodities()
{
    std::ifstream infile(_filename);
    if (!infile.is_open())
    {
        std::cerr << "无法打开文件：" << _filename << '\n';
        return false;
    }

    nlohmann::json j;
    infile >> j;
    for (const nlohmann::json &item : j)
    {
        std::string type(item["type"]);
        std::string name(item["name"]);
        std::string merchant(item["merchant"]);
        std::string description(item["description"]);
        double price = item["price"];
        int storage = item["storage"];
        double discount = item["discount"];

        addCommodity(type, name, merchant, description, price, storage, discount);
    }
    infile.close();
    return true;
}

bool CommodityManager::saveCommodities() const
{
    if (!_dirty)
        return true;
    std::ofstream outfile(_filename);
    if (!outfile.is_open())
    {
        std::cerr << "无法打开文件：" << _filename << '\n';
        return false;
    }

    nlohmann::json j;
    for (const auto &[name, commodity] : _nameMap)
    {
        nlohmann::json info = {
            {"type", commodity->getCommodityType()},
            {"name", commodity->getName()},
            {"description", commodity->getDescription()},
            {"merchant", commodity->getMerchant()},
            {"price", commodity->getBasePrice()},
            {"storage", commodity->getStorage()},
            {"discount", commodity->getDiscount()}};
        j.push_back(info);
    }
    outfile << j.dump(4);
    outfile.close();
    _dirty = false;
    return true;
}

bool CommodityManager::addCommodity(const std::string &type, const std::string &name, const std::string &merchant, const std::string &description, double price, int storage, double discount)
{
    if (type != "Food" && type != "Clothes" && type != "Book" && type != "Electronics")
    {
        std::cerr << "未知类型：" << type << '\n';
        return false;
    }
    if (_nameMap.find(name) != _nameMap.end())
    {
        std::cerr << name << "已经存在" << '\n';
        return false;
    }

    std::unique_ptr<Commodity> commodity;
    if (type == "Food")
    {
        commodity = std::make_unique<Food>(name, merchant, description, price, storage, discount);
    }
    else if (type == "Clothes")
    {
        commodity = std::make_unique<Clothes>(name, merchant, description, price, storage, discount);
    }
    else if (type == "Book")
    {
        commodity = std::make_unique<Book>(name, merchant, description, price, storage, discount);
    }
    else if (type == "Electronics")
    {
        commodity = std::make_unique<Electronics>(name, merchant, description, price, storage, discount);
    }
    _nameMap[name] = std::move(commodity);
    _dirty = true;
    return true;
}

void CommodityManager::showCommodities(const std::vector<Commodity *> &commodities) const
{
    const std::string separator(50, '-');
    for (const Commodity *commodity : commodities)
    {
        commodity->print();
    }
    std::cout << separator << '\n';
    std::cout << "共有 " << commodities.size() << " 个商品\n";
    std::cout << separator << '\n';
}

std::vector<const Commodity *> CommodityManager::findCommodity(const std::string &name) const
{
    // 如果搜索条件为空，则返回所有商品
    if (name.empty())
    {
        std::vector<const Commodity *> results;
        for (const auto &[n, c] : _nameMap)
            results.push_back(c.get());
        return results;
    }
    std::vector<const Commodity *> results;
    // 搜索包含指定名称的商品
    for (const auto &[n, commodity] : _nameMap)
    {
        if (commodity->getName().find(name) != std::string::npos)
        {
            results.push_back(commodity.get());
        }
    }
    return results;
}

std::vector<const Commodity *> CommodityManager::getCommodityByMerchant(const std::string &merchantName) const
{
    std::vector<const Commodity *> results;
    for (const auto &[n, commodity] : _nameMap)
    {
        if (commodity->getMerchant() == merchantName)
        {
            results.push_back(commodity.get());
        }
    }
    return results;
}

Commodity *CommodityManager::getCommodityByName(const std::string &name)
{
    auto it = _nameMap.find(name);
    return (it != _nameMap.end()) ? it->second.get() : nullptr;
}

std::vector<Commodity *> CommodityManager::getMutableCommodityByMerchant(const std::string &merchantName)
{
    std::vector<Commodity *> results;
    for (auto &[n, commodity] : _nameMap)
    {
        if (commodity->getMerchant() == merchantName)
        {
            results.push_back(commodity.get());
        }
    }
    return results;
}