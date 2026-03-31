#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include "Commodity.h"

#define MIN_PASSWORD_LENGTH 6  // 密码最小长度
#define MAX_PASSWORD_LENGTH 16 // 密码最大长度

// 用户基类，抽象类
class User
{
public:
    User(const std::string &username, const std::string &password, double balance = 0.0);
    virtual ~User() = default;

    virtual std::string getUserType() const = 0;                    // 获取用户类型，纯虚函数
    std::string getUsername() const;                                // 获取用户名
    std::string getPassword() const;                                // 获取密码（已加密）
    double getBalance() const;                                      // 获取余额
    std::string encryptPassword(const std::string &password) const; // 加密密码
    bool checkPassword(const std::string &password) const;          // 检查密码是否正确

    bool setPassword(const std::string &password); // 设置密码(会加密)
    bool addBalance(double amount);                // 充值
    bool subtractBalance(double amount);           // 消费

    virtual void showMenu(CommodityManager &commodityManager) = 0; // 显示菜单，纯虚函数
protected:
    std::string _username; // 用户名
    std::string _password; // 密码（已加密）
    double _balance;       // 余额
};

// 商家类，继承自User基类
class Merchant : public User
{
public:
    // 构造函数，初始化商家信息
    Merchant(const std::string &username, const std::string &password, double balance = 0.0)
        : User(username, password, balance) {}

    std::string getUserType() const override;                   // 返回商家用户类型为"Merchant"
    void showMenu(CommodityManager &commodityManager) override; // 显示商家菜单
};

// 用户类，继承自User基类
class Consumer : public User
{
public:
    // 构造函数，初始化用户信息
    Consumer(const std::string &username, const std::string &password, double balance = 0.0)
        : User(username, password, balance) {}

    std::string getUserType() const override;                   // 返回消费者用户类型为"Consumer"
    void showMenu(CommodityManager &commodityManager) override; // 显示用户菜单
};

// 用户管理类，负责用户的注册、登录和保存等操作
class UserManager
{
public:
    UserManager();
    ~UserManager();

    bool loadUsers();                                                                                     // 加载用户信息
    bool saveUsers() const;                                                                               // 保存用户信息
    bool registerUser(const std::string &type, const std::string &username, const std::string &password); // 注册用户
    User *login(const std::string &username, const std::string &password);                                // 用户登录，返回用户对象指针
private:
    std::vector<User *> _users;             // 用户列表
    std::string _filename = "./users.json"; // 用户数据存储文件名
};

#endif