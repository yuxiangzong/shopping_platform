#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include "ShoppingCart.h"

static constexpr int MIN_PASSWORD_LENGTH = 6;  // 密码最小长度
static constexpr int MAX_PASSWORD_LENGTH = 16; // 密码最大长度

class Order;

// 用户基类，抽象类
class User
{
public:
    User(const std::string &username, const std::string &password, double balance = 0.0)
        : _username(username), _password(password), _balance(balance) {}
    virtual ~User() = default;

    // 纯虚函数，返回用户类型字符串
    virtual std::string getUserType() const = 0;
    // 获取用户名
    std::string getUsername() const { return _username; }
    // 获取密码（已加密）
    std::string getPassword() const { return _password; }
    // 获取余额
    double getBalance() const { return _balance; }
    // 加密密码（使用std::hash实现简单哈希加密），静态方法不依赖对象状态
    static std::string encryptPassword(const std::string &password);
    // 检查密码是否正确
    bool checkPassword(const std::string &password) const { return encryptPassword(password) == _password; }

    // 设置密码，如果新密码长度不符合要求则返回false
    bool setPassword(const std::string &password);
    // 充值余额,如果金额小于0则返回false
    bool addBalance(double amount);
    // 消费余额，如果金额小于0或余额不足则返回false
    bool subtractBalance(double amount);

protected:
    std::string _username; // 用户名
    std::string _password; // 密码（已加密）
    double _balance;       // 余额
};

// 商家类，继承自User基类
class Merchant : public User
{
public:
    Merchant(const std::string &username, const std::string &password, double balance = 0.0)
        : User(username, password, balance) {}

    std::string getUserType() const override { return "Merchant"; }
};

// 消费者类，继承自User基类
class Consumer : public User
{
public:
    Consumer(const std::string &username, const std::string &password, double balance = 0.0)
        : User(username, password, balance) {}
    virtual ~Consumer();

    std::string getUserType() const override { return "Consumer"; }
    ShoppingCart _shoppingCart;   // 购物车
    std::vector<Order *> _orders; // 订单列表
};

// 用户管理类，负责用户的注册、登录和保存等操作
class UserManager
{
public:
    UserManager();
    ~UserManager();

    // 加载用户信息，从文件中读取并解析JSON数据到_users中
    bool loadUsers();
    // 保存用户信息，将_users中的数据序列化为JSON格式并写入文件
    bool saveUsers() const;
    // 注册用户，type为"Merchant"或"Consumer"，username和password为用户名和密码
    bool registerUser(const std::string &type, const std::string &username, const std::string &password);
    // 登录用户，返回User对象指针
    User *login(const std::string &username, const std::string &password);
    // 根据用户名获取用户对象
    User *getUserByUsername(const std::string &username) const;

private:
    std::vector<User *> _users;             // 用户列表
    std::string _filename = "./users.json"; // 用户数据存储文件名
};

#endif