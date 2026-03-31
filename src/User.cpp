#include <iostream>
#include <fstream>
#include "User.h"
#include "nlohmann/json.hpp"

// User类实现
User::User(const std::string &username, const std::string &password, double balance)
    : _username(username), _password(password), _balance(balance) {}
std::string User::getUsername() const { return _username; }
std::string User::getPassword() const { return _password; }
double User::getBalance() const { return _balance; }

std::string User::encryptPassword(const std::string &password) const
{
    // 使用std::hash实现简单哈希加密
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

bool User::checkPassword(const std::string &password) const
{
    return encryptPassword(password) == _password;
}

bool User::setPassword(const std::string &password)
{
    if (password.empty())
    {
        std::cerr << "密码不能为空" << std::endl;
        return false; // 密码无效，返回false
    }
    if (password.size() < MIN_PASSWORD_LENGTH || password.size() > MAX_PASSWORD_LENGTH)
    {
        std::cerr << "密码长度必须在" << MIN_PASSWORD_LENGTH << '~' << MAX_PASSWORD_LENGTH << "之间" << std::endl;
        return false; // 密码长度不符合要求
    }
    _password = encryptPassword(password); // 加密密码
    return true;                           // 设置成功，返回true
}

bool User::addBalance(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "充值金额必须大于零" << std::endl;
        return false; // 充值金额无效，返回false
    }
    _balance += amount;
    return true; // 充值成功，返回true
}

bool User::subtractBalance(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "消费金额必须大于零" << std::endl;
        return false; // 消费金额无效，返回false
    }
    if (_balance < amount)
    {
        std::cerr << "余额不足" << std::endl;
        return false; // 余额不足，无法消费，返回false
    }
    _balance -= amount;
    return true; // 消费成功，返回true
}

// Merchant类实现
std::string Merchant::getUserType() const { return "Merchant"; }
void Merchant::showMenu(CommodityManager &commodityManager)
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

        size_t choice;
        while (!(std::cin >> choice) || choice > 5)
        {
            std::cerr << "无效的选项，请重新输入: " << std::endl;
            std::cin.clear();
            std::cin.ignore(100, '\n');
        }
        std::cin.ignore(100, '\n');
        switch (choice)
        {
        case 0:
            return; // 退出登录
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
            commodityManager.manageCommodity(getUsername()); // 管理当前商家的商品
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

// Consumer类实现
std::string Consumer::getUserType() const { return "Consumer"; }

void Consumer::showMenu(CommodityManager &commodityManager)
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
        std::cout << "0. 退出登录\n";
        std::cout << "请选择: ";

        int choice;
        while (!(std::cin >> choice) || choice < 0 || choice > 5)
        {
            std::cerr << "无效的选项，请重新输入: " << std::endl;
            std::cin.clear();
            std::cin.ignore(100, '\n');
        }
        std::cin.ignore(100, '\n');
        switch (choice)
        {
        case 0:
            return; // 退出登录
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
        default:
            std::cerr << "无效的选项" << std::endl;
            break;
        }
    }
}

// UserManager类实现
UserManager::UserManager() { loadUsers(); }
UserManager::~UserManager()
{
    saveUsers();
    std::cout << "保存用户数据到文件：" << _filename << std::endl;
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
        return false; // 文件打开失败，加载失败
    }

    nlohmann::json j;
    infile >> j;
    for (const nlohmann::json &item : j)
    {
        if (item["type"] != "Merchant" && item["type"] != "Consumer")
        {
            std::cerr << "未知类型：" << item["type"] << std::endl;
            continue; // 忽略未知类型的用户
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
    return true; // 加载成功
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
    outfile << j.dump(4); // 输出缩进为4个空格的JSON格式
    outfile.close();
    return true; // 保存成功
}

bool UserManager::registerUser(const std::string &type, const std::string &username, const std::string &password)
{
    if (type != "Merchant" && type != "Consumer")
    {
        std::cerr << "未知类型：" << type << std::endl;
        return false; // 未知类型的用户
    }
    for (const User *user : _users)
    {
        if (user->getUsername() == username)
        {
            std::cerr << "用户名已存在" << std::endl;
            return false; // 用户名已存在
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
        return false; // 密码设置失败，删除新用户对象
    }
    _users.push_back(newUser);
    return true; // 注册成功
}

User *UserManager::login(const std::string &username, const std::string &password)
{
    for (User *user : _users)
    {
        if (user->getUsername() == username &&
            user->checkPassword(password))
        {
            return user; // 登录成功，返回用户对象
        }
    }
    std::cerr << "用户名或密码错误" << std::endl;
    return nullptr; // 登录失败，返回空指针
}