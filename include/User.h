#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include "ShoppingCart.h"

#define MIN_PASSWORD_LENGTH 6  // 密码最小长度
#define MAX_PASSWORD_LENGTH 16 // 密码最大长度

class UserManager;
class CommodityManager;
class Order;

// 用户基类，抽象类
class User
{
public:
    User(const std::string &username, const std::string &password, double balance = 0.0)
        : _username(username), _password(password), _balance(balance) {}
    virtual ~User() = default;

    virtual std::string getUserType() const = 0;
    std::string getUsername() const { return _username; }
    std::string getPassword() const { return _password; }
    double getBalance() const { return _balance; }
    std::string encryptPassword(const std::string &password) const;
    bool checkPassword(const std::string &password) const { return encryptPassword(password) == _password; }

    bool setPassword(const std::string &password);
    bool addBalance(double amount);
    bool subtractBalance(double amount);

    virtual void showMenu(CommodityManager &commodityManager, UserManager &userManager) = 0;

protected:
    std::string _username;
    std::string _password;
    double _balance;
};

// 商家类，继承自User基类
class Merchant : public User
{
public:
    Merchant(const std::string &username, const std::string &password, double balance = 0.0)
        : User(username, password, balance) {}

    std::string getUserType() const override { return "Merchant"; }
    void showMenu(CommodityManager &commodityManager, UserManager &userManager) override;
};

// 消费者类，继承自User基类
class Consumer : public User
{
public:
    Consumer(const std::string &username, const std::string &password, double balance = 0.0)
        : User(username, password, balance) {}

    std::string getUserType() const override { return "Consumer"; }
    void showMenu(CommodityManager &commodityManager, UserManager &userManager) override;

private:
    ShoppingCart _shoppingCart;
    std::vector<Order *> _orders;
};

// 用户管理类，负责用户的注册、登录和保存等操作
class UserManager
{
public:
    UserManager();
    ~UserManager();

    bool loadUsers();
    bool saveUsers() const;
    bool registerUser(const std::string &type, const std::string &username, const std::string &password);
    User *login(const std::string &username, const std::string &password);
    User *getUserByUsername(const std::string &username) const;

private:
    std::vector<User *> _users;
    std::string _filename = "./users.json";
};

#endif