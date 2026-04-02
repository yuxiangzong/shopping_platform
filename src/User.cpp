#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "User.h"
#include "Order.h"

// ==================== User ====================

std::string User::encryptPassword(const std::string &password)
{
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
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

// ==================== Consumer ====================

Consumer::~Consumer()
{
    for (Order *order : _orders)
    {
        delete order;
    }
}

// ==================== UserManager ====================

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
        return false; // 文件打开失败，加载失败
    }

    nlohmann::json j;
    infile >> j;
    for (const nlohmann::json &item : j)
    {
        std::string type(item["type"]);
        if (type != "Merchant" && type != "Consumer")
        {
            std::cerr << "未知类型：" << type << std::endl;
            continue; // 忽略未知类型的用户
        }
        std::string username(item["username"]);
        std::string password(item["password"]);
        double balance = item["balance"];

        User *user = (type == "Merchant")
                         ? static_cast<User *>(new Merchant(username, password, balance))
                         : static_cast<User *>(new Consumer(username, password, balance));
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

    User *newUser = (type == "Merchant")
                        ? static_cast<User *>(new Merchant(username, ""))
                        : static_cast<User *>(new Consumer(username, ""));
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

User *UserManager::getUserByUsername(const std::string &username) const
{
    for (User *user : _users)
    {
        if (user->getUsername() == username)
        {
            return user; // 返回非const指针，因为需要修改用户信息
        }
    }
    std::cerr << "无法找到用户名：" << username << std::endl;
    return nullptr; // 未找到用户
}