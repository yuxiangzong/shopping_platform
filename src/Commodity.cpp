#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "Commodity.h"

// ==================== Commodity ====================

bool Commodity::setBasePrice(double price)
{
    if (price < 0)
    {
        std::cerr << "价格不能为负数" << std::endl;
        return false;
    }
    _basePrice = price;
    return true;
}

bool Commodity::setDiscount(double discount)
{
    if (discount < 0 || discount > 1)
    {
        std::cerr << "折扣必须在0到1之间" << std::endl;
        return false;
    }
    _discount = discount;
    return true;
}

bool Commodity::setStorage(int storage)
{
    if (storage < 0)
    {
        std::cerr << "库存不能为负数" << std::endl;
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
    std::cout << "商品信息已保存" << std::endl;
    for (Commodity *commodity : _commodities)
    {
        delete commodity;
    }
}

bool CommodityManager::loadCommodities()
{
    std::ifstream infile(_filename);
    if (!infile.is_open())
    {
        std::cerr << "无法打开文件：" << _filename << std::endl;
        return false;
    }

    nlohmann::json j;
    infile >> j;
    for (const nlohmann::json &item : j)
    {
        if (item["type"] != "Food" && item["type"] != "Clothes" && item["type"] != "Book" && item["type"] != "Electronics")
        {
            std::cerr << "未知类型：" << item["type"] << std::endl;
            continue;
        }
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
    std::ofstream outfile(_filename);
    if (!outfile.is_open())
    {
        std::cerr << "无法打开文件：" << _filename << std::endl;
        return false;
    }

    nlohmann::json j;
    for (const Commodity *commodity : _commodities)
    {
        nlohmann::json info = {
            {"type", commodity->getCommodityType()},
            {"name", commodity->getName()},
            {"description", commodity->getDescription()},
            {"merchant", commodity->getMerchant()},
            {"price", commodity->getPrice()},
            {"storage", commodity->getStorage()},
            {"discount", commodity->getDiscount()}};
        j.push_back(info);
    }
    outfile << j.dump(4);
    outfile.close();
    return true;
}

bool CommodityManager::addCommodity(const std::string &type, const std::string &name, const std::string &merchant, const std::string &description, double price, int storage, double discount)
{
    if (type != "Food" && type != "Clothes" && type != "Book" && type != "Electronics")
    {
        std::cerr << "未知类型：" << type << std::endl;
        return false;
    }
    for (const Commodity *commodity : _commodities)
    {
        if (commodity->getName() == name)
        {
            std::cerr << name << "已经存在" << std::endl;
            return false;
        }
    }

    Commodity *commodity = nullptr;
    if (type == "Food")
    {
        commodity = new Food(name, merchant, description, price, storage, discount);
    }
    else if (type == "Clothes")
    {
        commodity = new Clothes(name, merchant, description, price, storage, discount);
    }
    else if (type == "Book")
    {
        commodity = new Book(name, merchant, description, price, storage, discount);
    }
    else if (type == "Electronics")
    {
        commodity = new Electronics(name, merchant, description, price, storage, discount);
    }
    _commodities.push_back(commodity);
    return true;
}

void CommodityManager::showCommodities(std::vector<Commodity *> commodities) const
{
    const std::string separator(50, '-');
    for (const Commodity *commodity : commodities)
    {
        commodity->print();
    }
    std::cout << separator << '\n';
    std::cout << "共有 " << commodities.size() << " 个商品\n";
    std::cout << separator << std::endl;
}

std::vector<Commodity *> CommodityManager::findCommodity(const std::string &name) const
{
    // 如果搜索条件为空，则返回所有商品
    if (name.empty())
    {
        return _commodities;
    }
    std::vector<Commodity *> results;
    // 搜索包含指定名称的商品
    for (const Commodity *commodity : _commodities)
    {
        if (commodity->getName().find(name) != std::string::npos)
        {
            results.push_back(const_cast<Commodity *>(commodity));
        }
    }
    return results;
}

std::vector<Commodity *> CommodityManager::getCommodityByMerchant(const std::string &merchantName) const
{
    std::vector<Commodity *> results;
    for (const Commodity *commodity : _commodities)
    {
        if (commodity->getMerchant() == merchantName)
        {
            results.push_back(const_cast<Commodity *>(commodity));
        }
    }
    return results;
}
