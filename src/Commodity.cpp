#include <iostream>
#include <fstream>
#include "Commodity.h"
#include "nlohmann/json.hpp"

// Commodity类实现
Commodity::Commodity(const std::string &name, const std::string &merchant, const std::string &description, double basePrice, int storage, double discount)
    : _name(name), _description(description), _merchant(merchant), _basePrice(basePrice), _discount(discount), _storage(storage) {}

std::string Commodity::getName() const { return _name; }
std::string Commodity::getDescription() const { return _description; }
std::string Commodity::getMerchant() const { return _merchant; }
double Commodity::getBasePrice() const { return _basePrice; }
double Commodity::getPrice() const { return _basePrice * _discount; }
double Commodity::getDiscount() const { return _discount; }
int Commodity::getStorage() const { return _storage; }

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

// Food类实现
std::string Food::getCommodityType() const { return "Food"; }

// Clothes类实现
std::string Clothes::getCommodityType() const { return "Clothes"; }

// Book类实现
std::string Book::getCommodityType() const { return "Book"; }

// Electronics类实现
std::string Electronics::getCommodityType() const { return "Electronics"; }

// CommodityManager类实现
CommodityManager::CommodityManager() { loadCommodities(); }
CommodityManager::~CommodityManager()
{
    saveCommodities();
    std::cout << "保存商品数据到文件：" << _filename << std::endl;
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
        if (commodity->getName() == name && commodity->getCommodityType() == type)
        {
            std::cerr << name << " 已存在于" << type << "类型中, 无法添加" << std::endl;
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
    for (Commodity *commodity : _commodities)
    {
        if (commodity->getName().find(name) != std::string::npos || commodity->getMerchant().find(name) != std::string::npos)
        {
            results.push_back(commodity);
        }
    }
    return results;
}

void CommodityManager::manageCommodity(const std::string &merchant)
{
    std::cout << "\n===== 商品信息管理菜单 =====\n";
    std::vector<Commodity *> my_commodities = findCommodity(merchant); // 查找属于当前用户的商品
    if (my_commodities.empty())
    {
        std::cout << "当前没有可管理的商品" << std::endl;
        return;
    }
    std::cout << "选择要管理的商品:\n";
    for (size_t i = 0; i < my_commodities.size(); ++i)
    {
        std::cout << i + 1 << ". " << my_commodities[i]->getName() << " (" << my_commodities[i]->getCommodityType() << ")" << std::endl;
    }
    std::cout << "输入商品编号（0 退出）: ";
    size_t choice;

    while (!(std::cin >> choice) || choice > my_commodities.size())
    {
        std::cin.clear();
        std::cin.ignore(100, '\n');
        std::cerr << "无效的输入" << std::endl;
        std::cout << "输入商品编号（0 退出）: ";
    }
    if (choice == 0)
        return;
    Commodity *selected = my_commodities[choice - 1];
    while (true)
    {
        std::cout << "\n当前商品: " << selected->getName() << "，类型：" << selected->getCommodityType() << std::endl;
        std::cout << "1. 修改原价\n";
        std::cout << "2. 修改库存\n";
        std::cout << "3. 修改折扣\n";
        std::cout << "4. 查看商品详细信息\n";
        std::cout << "0. 退出商品信息管理\n";
        std::cout << "选择要执行的操作: ";
        while (!(std::cin >> choice) || choice > 4)
        {
            std::cin.clear();
            std::cin.ignore(100, '\n');
            std::cerr << "无效的输入" << std::endl;
            std::cout << "选择要执行的操作: ";
        }
        switch (choice)
        {
        case 0:
            return;
        case 1:
        {
            std::cout << "当前原价：" << selected->getPrice() << std::endl;
            double newPrice;
            std::cout << "输入新原价: ";
            if (!(std::cin >> newPrice))
            {
                std::cerr << "无效的输入" << std::endl;
                continue;
            }
            if (selected->setBasePrice(newPrice))
            {
                std::cout << "原价修改成功，现在为: " << selected->getPrice() << std::endl;
            }
            break;
        }
        case 2:
        {
            std::cout << "当前库存：" << selected->getStorage() << std::endl;
            int newStorage;
            std::cout << "输入新库存: ";
            if (!(std::cin >> newStorage))
            {
                std::cerr << "无效的输入" << std::endl;
                continue;
            }
            if (selected->setStorage(newStorage))
            {
                std::cout << "库存修改成功，现在为: " << selected->getStorage() << std::endl;
            }
            break;
        }
        case 3:
        {
            int choice;
            std::cout << "1. 单个商品打折\n"
                      << "2. 相同类型商品批量打折\n";
            std::cout << "选择要执行的操作: ";
            while (!(std::cin >> choice) || choice < 1 || choice > 2)
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cerr << "无效的输入" << std::endl;
                std::cout << "选择要执行的操作: ";
            }
            std::cin.ignore(100, '\n');
            if (choice == 1)
            {
                std::cout << selected->getName() << "当前折扣：" << selected->getDiscount() << std::endl;
                double newDiscount;
                std::cout << "输入新折扣（0~1，1为原价）: ";
                if (!(std::cin >> newDiscount))
                {
                    std::cerr << "无效的输入" << std::endl;
                    continue;
                }
                if (selected->setDiscount(newDiscount))
                {
                    std::cout << "折扣修改成功，现在为: " << selected->getDiscount() << std::endl;
                }
                else
                {
                    std::cout << "折扣修改失败" << std::endl;
                }
            }
            else
            {
                std::cout << "批量打折类型为 " << selected->getCommodityType() << std::endl;
                double newDiscount;
                std::cout << "输入新折扣（0~1，1为原价）: ";
                if (!(std::cin >> newDiscount))
                {
                    std::cerr << "无效的输入" << std::endl;
                    continue;
                }
                for (Commodity *&commodity : my_commodities)
                {
                    if (commodity->getCommodityType() == selected->getCommodityType())
                    {
                        std::cout << commodity->getName() << "的折扣修改中...，原来折扣为：" << commodity->getDiscount() << std::endl;
                        if (commodity->setDiscount(newDiscount))
                        {
                            std::cout << "折扣修改成功，现在为: " << commodity->getDiscount() << std::endl;
                        }
                        else
                        {
                            std::cout << commodity->getName() << "折扣修改失败" << std::endl;
                        }
                    }
                }
            }
            break;
        }
        case 4:
            selected->print();
            break;
        default:
            std::cout << "无效的选项" << std::endl;
            break;
        }
    }
}