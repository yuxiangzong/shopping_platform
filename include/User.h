#ifndef USER_H
#define USER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <sqlite3.h>

static constexpr int MIN_PASSWORD_LENGTH = 6;
static constexpr int MAX_PASSWORD_LENGTH = 16;

// 用户基类，抽象类
class User
{
public:
    User(int id, const std::string &username, const std::string &password, double balance = 0.0)
        : _id(id), _username(username), _password(password), _balance(balance) {}
    virtual ~User() = default;

    virtual std::string getUserType() const = 0;

    int getId() const { return _id; }
    std::string getUsername() const { return _username; }
    std::string getPassword() const { return _password; }
    double getBalance() const { return _balance; }

    static std::string encryptPassword(const std::string &password);
    bool checkPassword(const std::string &password) const { return encryptPassword(password) == _password; }

    // 以下方法仅更新内存，DB 持久化由 UserManager 负责
    bool setPassword(const std::string &password);
    bool addBalance(double amount);
    bool subtractBalance(double amount);

protected:
    int _id;
    std::string _username;
    std::string _password;
    double _balance;
};

// 商家类
class Merchant : public User
{
public:
    using User::User;
    std::string getUserType() const override { return "Merchant"; }
};

// 消费者类
class Consumer : public User
{
public:
    using User::User;
    std::string getUserType() const override { return "Consumer"; }
};

// 用户管理类（按需缓存：查缓存未命中时从 DB 加载）
class UserManager
{
public:
    explicit UserManager(sqlite3 *db);
    ~UserManager() = default;
    UserManager(const UserManager &) = delete;
    UserManager &operator=(const UserManager &) = delete;

    bool registerUser(const std::string &type, const std::string &username, const std::string &password);
    User *login(const std::string &username, const std::string &password);
    User *getUserByUsername(const std::string &username);

    User *getUserById(int userId);
    bool updateBalance(User *user);
    bool updatePassword(User *user);

private:
    sqlite3 *_db;
    std::mutex _cacheMutex;
    std::unordered_map<std::string, std::unique_ptr<User>> _userMap;

    // 查缓存，未命中则从 DB 加载并加入缓存
    User *getOrLoad(const std::string &username);
};

#endif
