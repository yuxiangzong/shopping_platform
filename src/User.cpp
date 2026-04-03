#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "User.h"
#include "Order.h"

// ==================== User ====================

std::string User::encryptPassword(const std::string &password)
{
    // 加盐 + 多轮迭代哈希，确保跨平台一致性和安全性
    static const std::string salt = "ShoppingPlatform2025";
    static constexpr int ITERATIONS = 1000;
    std::hash<std::string> hasher;
    size_t result = hasher(salt + password);
    for (int i = 0; i < ITERATIONS; ++i)
    {
        result = hasher(salt + std::to_string(result) + password);
    }
    return std::to_string(result);
}

bool User::setPassword(const std::string &password)
{
    if (password.empty())
    {
        std::cerr << "密码不能为空" << '\n';
        return false; // 密码无效，返回false
    }
    if (password.size() < MIN_PASSWORD_LENGTH || password.size() > MAX_PASSWORD_LENGTH)
    {
        std::cerr << "密码长度必须在" << MIN_PASSWORD_LENGTH << '~' << MAX_PASSWORD_LENGTH << "之间" << '\n';
        return false; // 密码长度不符合要求
    }
    _password = encryptPassword(password); // 加密密码
    return true;                           // 设置成功，返回true
}

bool User::addBalance(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "充值金额必须大于零" << '\n';
        return false; // 充值金额无效，返回false
    }
    _balance += amount;
    return true; // 充值成功，返回true
}

bool User::subtractBalance(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "消费金额必须大于零" << '\n';
        return false; // 消费金额无效，返回false
    }
    if (_balance < amount)
    {
        std::cerr << "余额不足" << '\n';
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
    std::cout << "用户信息已保存" << '\n';
    for (auto &[username, user] : _userMap)
    {
        delete user;
    }
}

bool UserManager::loadUsers()
{
    std::ifstream infile(_filename);
    if (!infile.is_open())
    {
        std::cerr << "无法打开文件：" << _filename << '\n';
        return false; // 文件打开失败，加载失败
    }

    nlohmann::json j;
    infile >> j;
    for (const nlohmann::json &item : j)
    {
        std::string type(item["type"]);
        if (type != "Merchant" && type != "Consumer")
        {
            std::cerr << "未知类型：" << type << '\n';
            continue; // 忽略未知类型的用户
        }
        std::string username(item["username"]);
        std::string password(item["password"]);
        double balance = item["balance"];

        User *user = (type == "Merchant")
                         ? static_cast<User *>(new Merchant(username, password, balance))
                         : static_cast<User *>(new Consumer(username, password, balance));
        _userMap[username] = user;
    }
    infile.close();
    return true; // 加载成功
}

bool UserManager::saveUsers() const
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
    for (const auto &[username, user] : _userMap)
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
    _dirty = false;
    return true; // 保存成功
}

bool UserManager::registerUser(const std::string &type, const std::string &username, const std::string &password)
{
    if (type != "Merchant" && type != "Consumer")
    {
        std::cerr << "未知类型：" << type << '\n';
        return false; // 未知类型的用户
    }
    if (_userMap.find(username) != _userMap.end())
    {
        std::cerr << "用户名已存在" << '\n';
        return false; // 用户名已存在
    }

    User *newUser = (type == "Merchant")
                        ? static_cast<User *>(new Merchant(username, ""))
                        : static_cast<User *>(new Consumer(username, ""));
    if (!newUser->setPassword(password))
    {
        delete newUser;
        return false; // 密码设置失败，删除新用户对象
    }
    _userMap[username] = newUser;
    _dirty = true;
    return true; // 注册成功
}

User *UserManager::login(const std::string &username, const std::string &password)
{
    auto it = _userMap.find(username);
    if (it != _userMap.end() && it->second->checkPassword(password))
    {
        return it->second; // 登录成功，返回用户对象
    }
    std::cerr << "用户名或密码错误" << '\n';
    return nullptr; // 登录失败，返回空指针
}

User *UserManager::getUserByUsername(const std::string &username) const
{
    auto it = _userMap.find(username);
    if (it != _userMap.end())
    {
        return it->second; // 返回非const指针，因为需要修改用户信息
    }
    std::cerr << "无法找到用户名：" << username << '\n';
    return nullptr; // 未找到用户
}