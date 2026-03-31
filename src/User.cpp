#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "User.h"
#include "Commodity.h"
#include "Order.h"

std::string User::encryptPassword(const std::string &password) const
{
    // 使用std::hash实现简单哈希加密
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

bool User::setPassword(const std::string &password)
{
    if (password.empty())
    {
        std::cerr << "密码不能为空" << std::endl;
        return false;
    }
    if (password.size() < MIN_PASSWORD_LENGTH || password.size() > MAX_PASSWORD_LENGTH)
    {
        std::cerr << "密码长度必须在" << MIN_PASSWORD_LENGTH << '~' << MAX_PASSWORD_LENGTH << "之间" << std::endl;
        return false;
    }
    _password = encryptPassword(password);
    return true;
}

bool User::addBalance(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "充值金额必须大于零" << std::endl;
        return false;
    }
    _balance += amount;
    return true;
}

bool User::subtractBalance(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "消费金额必须大于零" << std::endl;
        return false;
    }
    if (_balance < amount)
    {
        std::cerr << "余额不足" << std::endl;
        return false;
    }
    _balance -= amount;
    return true;
}

void Merchant::showMenu(CommodityManager &commodityManager, UserManager &userManager)
{
    while (true)
    {
        std::cout << "\n****** 商家菜单 ******\n";
        std::cout << "欢迎，商家：" << getUsername() << std::endl;
        std::cout << "1. 查看余额\n";
        std::cout << "2. 充值\n";
        std::cout << "3. 添加商品\n";
        std::cout << "4. 管理商品\n";
        std::cout << "5. 修改密码\n";
        std::cout << "0. 退出登录\n";
        std::cout << "请选择: ";

        int choice;
        while (!(std::cin >> choice) || choice < 0 || choice > 6)
        {
            std::cerr << "无效的选项，请重新输入: " << std::endl;
            std::cin.clear();
            std::cin.ignore(100, '\n');
        }
        std::cin.ignore(100, '\n');
        switch (choice)
        {
        case 0:
            return;
        case 1:
            std::cout << "\n当前余额: " << getBalance() << std::endl;
            break;
        case 2:
        {
            std::cout << "\n当前余额: " << getBalance() << std::endl;
            double amount;
            std::cout << "请输入充值金额: ";
            std::cin >> amount;
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cerr << "输入不合法" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');
            if (addBalance(amount))
            {
                std::cout << "充值成功，当前余额: " << getBalance() << std::endl;
            }
        }
        break;
        case 3:
        {
            std::cout << "请输入商品类型ID(1: Food, 2: Clothes, 3: Book, 4: Electronics): ";
            short typeID;
            std::cin >> typeID;
            if (std::cin.fail() || typeID < 1 || typeID > 4)
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cerr << "无效的商品类型" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');
            std::string type, name, description;
            switch (typeID)
            {
            case 1:
                type = "Food";
                break;
            case 2:
                type = "Clothes";
                break;
            case 3:
                type = "Book";
                break;
            case 4:
                type = "Electronics";
                break;
            }
            std::cout << "请输入商品名称: ";
            std::getline(std::cin >> std::ws, name);
            std::cout << "请输入商品描述: ";
            std::getline(std::cin >> std::ws, description);
            std::cout << "请输入商品价格: ";
            double price;
            std::cin >> price;
            if (std::cin.fail() || price <= 0)
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cout << "输入不合法，价格必须是正数" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');
            std::cout << "请输入商品库存: ";
            int stock;
            std::cin >> stock;
            if (std::cin.fail() || stock <= 0)
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cout << "输入不合法" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');
            if (commodityManager.addCommodity(type, name, getUsername(), description, price, stock))
            {
                std::cout << "商品：" << name << "添加成功" << std::endl;
            }
        }
        break;
        case 4:
            commodityManager.manageCommodity(getUsername());
            break;
        case 5:
        {
            std::cout << "请输入新密码: ";
            std::string newPassword;
            std::getline(std::cin, newPassword);
            if (setPassword(newPassword))
            {
                std::cout << "密码设置成功" << std::endl;
            }
        }
        break;
        default:
            std::cerr << "无效的选项" << std::endl;
            break;
        }
    }
}

void Consumer::showMenu(CommodityManager &commodityManager, UserManager &userManager)
{
    while (true)
    {
        std::cout << "\n****** 消费者菜单 ******\n";
        std::cout << "欢迎，用户：" << getUsername() << std::endl;
        std::cout << "1. 查看余额\n";
        std::cout << "2. 充值\n";
        std::cout << "3. 查看商品\n";
        std::cout << "4. 搜索商品\n";
        std::cout << "5. 修改密码\n";
        std::cout << "6. 购物车\n";
        std::cout << "7. 待支付订单\n";
        std::cout << "0. 退出登录\n";
        std::cout << "请选择: ";

        int choice;
        while (!(std::cin >> choice) || choice < 0 || choice > 7)
        {
            std::cerr << "无效的选项，请重新输入: " << std::endl;
            std::cin.clear();
            std::cin.ignore(100, '\n');
        }
        std::cin.ignore(100, '\n');
        switch (choice)
        {
        case 0:
            return;
        case 1:
            std::cout << "\n当前余额: " << getBalance() << std::endl;
            break;
        case 2:
        {
            std::cout << "\n当前余额: " << getBalance() << std::endl;
            double amount;
            std::cout << "请输入充值金额: ";
            std::cin >> amount;
            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(100, '\n');
                std::cerr << "输入不合法" << std::endl;
                break;
            }
            std::cin.ignore(100, '\n');
            if (addBalance(amount))
            {
                std::cout << "充值成功，当前余额: " << getBalance() << std::endl;
            }
        }
        break;
        case 3:
        {
            std::vector<Commodity *> commodities = commodityManager.findCommodity("");
            commodityManager.showCommodities(commodities);
            break;
        }
        case 4:
        {
            std::string name;
            std::cout << "请输入商品名称: ";
            std::getline(std::cin >> std::ws, name);
            std::vector<Commodity *> commodities = commodityManager.findCommodity(name);
            commodityManager.showCommodities(commodities);
        }
        break;
        case 5:
        {
            std::string newPassword;
            std::cout << "请输入新密码: ";
            std::getline(std::cin, newPassword);
            if (setPassword(newPassword))
            {
                std::cout << "密码设置成功" << std::endl;
            }
        }
        break;
        case 6:
        {
            while (true)
            {
                std::cout << "\n****** 购物车 ******\n";
                std::cout << "1. 查看购物车\n";
                std::cout << "2. 添加商品到购物车\n";
                std::cout << "3. 从购物车移除商品\n";
                std::cout << "4. 修改商品数量\n";
                std::cout << "5. 结算购物车\n";
                std::cout << "0. 返回上一级\n";
                std::cout << "请选择: ";

                int cartChoice;
                while (!(std::cin >> cartChoice) || cartChoice < 0 || cartChoice > 5)
                {
                    std::cerr << "无效的选项，请重新输入: " << std::endl;
                    std::cin.clear();
                    std::cin.ignore(100, '\n');
                }
                std::cin.ignore(100, '\n');
                switch (cartChoice)
                {
                case 0:
                    goto end_shopping_cart;
                case 1:
                {
                    auto items = _shoppingCart.getItems();
                    if (items.empty())
                    {
                        std::cout << "购物车为空" << std::endl;
                    }
                    else
                    {
                        std::cout << "购物车商品列表:" << std::endl;
                        for (const auto &item : items)
                        {
                            std::cout << item.first->getName() << " x" << item.second
                                      << " 单价:" << item.first->getPrice()
                                      << " 小计:" << item.first->getPrice() * item.second << std::endl;
                        }
                        std::cout << "总计: " << _shoppingCart.calculateTotal() << std::endl;
                    }
                    break;
                }
                case 2:
                {
                    std::string name;
                    std::cout << "请输入要添加的商品名称: ";
                    std::getline(std::cin >> std::ws, name);
                    auto commodities = commodityManager.findCommodity(name);
                    if (commodities.empty())
                    {
                        std::cout << "未找到商品" << std::endl;
                    }
                    else if (commodities.size() > 1)
                    {
                        std::cout << "找到多个商品，请选择:" << std::endl;
                        for (size_t i = 0; i < commodities.size(); ++i)
                        {
                            std::cout << i + 1 << ". " << commodities[i]->getName()
                                      << " 价格:" << commodities[i]->getPrice()
                                      << " 库存:" << commodities[i]->getStorage()
                                      << " 商家:" << commodities[i]->getMerchant() << std::endl;
                        }
                        size_t selection;
                        std::cin >> selection;
                        std::cin.ignore(100, '\n');
                        if (selection < 1 || selection > commodities.size())
                        {
                            std::cout << "无效选择" << std::endl;
                            break;
                        }
                        Commodity *selected = commodities[selection - 1];
                        int quantity;
                        std::cout << "请输入购买数量: ";
                        std::cin >> quantity;
                        std::cin.ignore(100, '\n');
                        if (_shoppingCart.addItem(selected, quantity))
                        {
                            std::cout << "添加成功" << std::endl;
                        }
                    }
                    else
                    {
                        Commodity *selected = commodities[0];
                        int quantity;
                        std::cout << selected->getName()
                                  << " 价格:" << selected->getPrice()
                                  << " 库存:" << selected->getStorage() << "，请输入购买数量: ";
                        std::cin >> quantity;
                        std::cin.ignore(100, '\n');
                        if (_shoppingCart.addItem(selected, quantity))
                        {
                            std::cout << "添加成功" << std::endl;
                        }
                    }
                    break;
                }
                case 3:
                {
                    auto items = _shoppingCart.getItems();
                    if (items.empty())
                    {
                        std::cout << "购物车为空" << std::endl;
                    }
                    else
                    {
                        std::cout << "请选择要移除的商品:" << std::endl;
                        for (size_t i = 0; i < items.size(); ++i)
                        {
                            std::cout << i + 1 << ". " << items[i].first->getName()
                                      << " x" << items[i].second << std::endl;
                        }
                        size_t selection;
                        std::cin >> selection;
                        std::cin.ignore(100, '\n');
                        if (selection < 1 || selection > items.size())
                        {
                            std::cout << "无效选择" << std::endl;
                            break;
                        }
                        if (_shoppingCart.removeItem(items[selection - 1].first))
                        {
                            std::cout << "移除成功" << std::endl;
                        }
                    }
                    break;
                }
                case 4:
                {
                    auto items = _shoppingCart.getItems();
                    if (items.empty())
                    {
                        std::cout << "购物车为空" << std::endl;
                    }
                    else
                    {
                        std::cout << "请选择要修改的商品:" << std::endl;
                        for (size_t i = 0; i < items.size(); ++i)
                        {
                            std::cout << i + 1 << ". " << items[i].first->getName()
                                      << " 当前数量: x" << items[i].second << std::endl;
                        }
                        size_t selection;
                        std::cin >> selection;
                        std::cin.ignore(100, '\n');
                        if (selection < 1 || selection > items.size())
                        {
                            std::cout << "无效选择" << std::endl;
                            break;
                        }
                        int newQuantity;
                        std::cout << "请输入新的数量: ";
                        std::cin >> newQuantity;
                        std::cin.ignore(100, '\n');
                        if (_shoppingCart.updateQuantity(items[selection - 1].first, newQuantity))
                        {
                            std::cout << "修改成功" << std::endl;
                        }
                    }
                    break;
                }
                case 5:
                {
                    auto items = _shoppingCart.getItems();
                    if (items.empty())
                    {
                        std::cout << "购物车为空" << std::endl;
                        break;
                    }
                    // 创建订单
                    Order *order = new Order(items);
                    std::cout << "订单总金额: " << order->getTotalAmount() << std::endl;
                    std::cout << "确认结算? (y/n): ";
                    char confirm;
                    std::cin >> confirm;
                    std::cin.ignore(100, '\n');
                    if (confirm == 'y' || confirm == 'Y')
                    {
                        if (order->pay(this, &userManager))
                        {
                            std::cout << "支付成功!" << std::endl;
                            delete order;
                            _shoppingCart.clear();
                        }
                        else
                        {
                            _orders.push_back(order);
                            _shoppingCart.clear();
                            std::cout << "订单已保存" << std::endl;
                        }
                    }
                    else
                    {
                        _orders.push_back(order);
                        _shoppingCart.clear();
                        std::cout << "订单已保存" << std::endl;
                    }
                    break;
                }
                default:
                    std::cerr << "无效的选项" << std::endl;
                    break;
                }
            }
        end_shopping_cart:
            break;
        }
        case 7:
        {
            while (true)
            {
                if (_orders.empty())
                {
                    std::cout << "没有待支付订单\n";
                    break;
                }
                std::cout << "\n****** 待支付订单 ******\n";
                for (size_t i = 0; i < _orders.size(); ++i)
                {
                    std::cout << i + 1 << ". 订单总金额: " << _orders[i]->getTotalAmount() << "\n";
                }

                std::cout << "请选择要操作的订单(0退出): ";
                size_t selection;
                std::cin >> selection;
                std::cin.ignore(100, '\n');

                if (selection == 0)
                {
                    break;
                }
                else if (selection < 1 || selection > _orders.size())
                {
                    std::cerr << "无效选择\n";
                    continue;
                }

                Order *selectedOrder = _orders[selection - 1];
                if (selectedOrder->getStatus() != Order::PENDING)
                {
                    std::cerr << "该订单不是待支付状态\n";
                    continue;
                }

                std::cout << "\n订单详情:\n";
                for (const auto &item : selectedOrder->getItems())
                {
                    std::cout << item.first->getName() << " x" << item.second
                              << " 单价:" << item.first->getPrice()
                              << " 小计:" << item.first->getPrice() * item.second << std::endl;
                }
                std::cout << "总金额: " << selectedOrder->getTotalAmount() << "\n";
                std::cout << "1. 支付订单\n";
                std::cout << "2. 取消订单\n";
                std::cout << "请选择操作: ";

                int action;
                std::cin >> action;
                std::cin.ignore(100, '\n');

                switch (action)
                {
                case 1:
                    if (selectedOrder->pay(this, &userManager))
                    {
                        std::cout << "支付成功\n";
                        delete selectedOrder;
                        _orders.erase(_orders.begin() + (selection - 1));
                    }
                    break;
                case 2:
                    if (selectedOrder->cancel())
                    {
                        std::cout << "订单已取消\n";
                        delete selectedOrder;
                        _orders.erase(_orders.begin() + (selection - 1));
                    }
                    break;
                default:
                    std::cerr << "无效操作\n";
                    break;
                }
            }
            break;
        }
        default:
            std::cerr << "无效的选项" << std::endl;
            break;
        }
    }
}

UserManager::UserManager() { loadUsers(); }

UserManager::~UserManager()
{
    saveUsers();
    std::cout << "用户信息已保存" << std::endl;
    for (User *user : _users)
    {
        delete user;
    }
}

bool UserManager::loadUsers()
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
        if (item["type"] != "Merchant" && item["type"] != "Consumer")
        {
            std::cerr << "未知类型：" << item["type"] << std::endl;
            continue;
        }
        std::string type(item["type"]);
        std::string username(item["username"]);
        std::string password(item["password"]);
        double balance = item["balance"];

        User *user = nullptr;
        if (type == "Merchant")
        {
            user = new Merchant(username, password, balance);
        }
        else if (type == "Consumer")
        {
            user = new Consumer(username, password, balance);
        }
        _users.push_back(user);
    }
    infile.close();
    return true;
}

bool UserManager::saveUsers() const
{
    std::ofstream outfile(_filename);
    if (!outfile.is_open())
    {
        std::cerr << "无法打开文件：" << _filename << std::endl;
        return false;
    }

    nlohmann::json j;
    for (const User *user : _users)
    {
        nlohmann::json info = {
            {"type", user->getUserType()},
            {"username", user->getUsername()},
            {"password", user->getPassword()},
            {"balance", user->getBalance()}};
        j.push_back(info);
    }
    outfile << j.dump(4);
    outfile.close();
    return true;
}

bool UserManager::registerUser(const std::string &type, const std::string &username, const std::string &password)
{
    if (type != "Merchant" && type != "Consumer")
    {
        std::cerr << "未知类型：" << type << std::endl;
        return false;
    }
    for (const User *user : _users)
    {
        if (user->getUsername() == username)
        {
            std::cerr << "用户名已存在" << std::endl;
            return false;
        }
    }

    User *newUser = nullptr;
    if (type == "Merchant")
    {
        newUser = new Merchant(username, "");
    }
    else if (type == "Consumer")
    {
        newUser = new Consumer(username, "");
    }
    if (!newUser->setPassword(password))
    {
        delete newUser;
        return false;
    }
    _users.push_back(newUser);
    return true;
}

User *UserManager::login(const std::string &username, const std::string &password)
{
    for (User *user : _users)
    {
        if (user->getUsername() == username &&
            user->checkPassword(password))
        {
            return user;
        }
    }
    std::cerr << "用户名或密码错误" << std::endl;
    return nullptr;
}

User *UserManager::getUserByUsername(const std::string &username) const
{
    for (const User *user : _users)
    {
        if (user->getUsername() == username)
        {
            return const_cast<User *>(user);
        }
    }
    std::cerr << "无法找到用户名：" << username << std::endl;
    return nullptr;
}
