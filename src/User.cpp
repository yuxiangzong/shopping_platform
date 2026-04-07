#include <iostream>
#include <sqlite3.h>
#include "User.h"

// ==================== User ====================

std::string User::encryptPassword(const std::string &password)
{
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
        return false;
    }
    if (password.size() < MIN_PASSWORD_LENGTH || password.size() > MAX_PASSWORD_LENGTH)
    {
        std::cerr << "密码长度必须在" << MIN_PASSWORD_LENGTH << '~' << MAX_PASSWORD_LENGTH << "之间" << '\n';
        return false;
    }
    _password = encryptPassword(password);
    return true;
}

bool User::addBalance(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "充值金额必须大于零" << '\n';
        return false;
    }
    _balance += amount;
    return true;
}

bool User::subtractBalance(double amount)
{
    if (amount <= 0)
    {
        std::cerr << "消费金额必须大于零" << '\n';
        return false;
    }
    if (_balance < amount)
    {
        std::cerr << "余额不足" << '\n';
        return false;
    }
    _balance -= amount;
    return true;
}

// ==================== UserManager ====================

UserManager::UserManager(sqlite3 *db) : _db(db) {}

User *UserManager::getOrLoad(const std::string &username)
{
    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        auto it = _userMap.find(username);
        if (it != _userMap.end())
            return it->second.get();
    }

    // 缓存未命中，从 DB 加载
    const char *sql = "SELECT id, password, type, balance FROM users WHERE username = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return nullptr;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::unique_ptr<User> loaded;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        std::string pwd(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        std::string type(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        double balance = sqlite3_column_double(stmt, 3);

        if (type == "Merchant")
            loaded = std::make_unique<Merchant>(id, username, pwd, balance);
        else
            loaded = std::make_unique<Consumer>(id, username, pwd, balance);
    }
    sqlite3_finalize(stmt);

    if (!loaded) return nullptr;

    // 加入缓存（双重检查防止覆盖）
    std::lock_guard<std::mutex> lock(_cacheMutex);
    auto &slot = _userMap[username];
    if (slot)
        return slot.get(); // 另一个线程抢先加载了
    slot = std::move(loaded);
    return slot.get();
}

bool UserManager::registerUser(const std::string &type, const std::string &username, const std::string &password)
{
    if (type != "Merchant" && type != "Consumer")
    {
        std::cerr << "未知类型：" << type << '\n';
        return false;
    }

    // 先检查缓存
    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        if (_userMap.count(username))
        {
            std::cerr << "用户名已存在" << '\n';
            return false;
        }
    }

    if (password.empty() || password.size() < MIN_PASSWORD_LENGTH || password.size() > MAX_PASSWORD_LENGTH)
    {
        std::cerr << "密码长度必须在" << MIN_PASSWORD_LENGTH << '~' << MAX_PASSWORD_LENGTH << "之间" << '\n';
        return false;
    }
    std::string encrypted = User::encryptPassword(password);

    // 写入 DB（UNIQUE 约束保证最终一致性）
    const char *sql = "INSERT INTO users (username, password, type, balance) VALUES (?, ?, ?, 0.0)";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "SQL prepare failed: " << sqlite3_errmsg(_db) << '\n';
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, encrypted.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "Register failed: " << sqlite3_errmsg(_db) << '\n';
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);

    int id = static_cast<int>(sqlite3_last_insert_rowid(_db));
    std::unique_ptr<User> user;
    if (type == "Merchant")
        user = std::make_unique<Merchant>(id, username, encrypted);
    else
        user = std::make_unique<Consumer>(id, username, encrypted);

    std::lock_guard<std::mutex> lock(_cacheMutex);
    _userMap[username] = std::move(user);
    return true;
}

User *UserManager::login(const std::string &username, const std::string &password)
{
    User *user = getOrLoad(username);
    if (user && user->checkPassword(password))
        return user;
    return nullptr;
}

User *UserManager::getUserByUsername(const std::string &username)
{
    return getOrLoad(username);
}

User *UserManager::getUserById(int userId)
{
    // 先在缓存中按 id 查找
    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        for (const auto &[name, user] : _userMap)
        {
            if (user->getId() == userId)
                return user.get();
        }
    }

    // 缓存未命中，从 DB 加载
    const char *sql = "SELECT username FROM users WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return nullptr;
    sqlite3_bind_int(stmt, 1, userId);
    std::string username;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);

    if (username.empty()) return nullptr;
    return getOrLoad(username);
}

bool UserManager::updateBalance(User *user)
{
    const char *sql = "UPDATE users SET balance = ? WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_double(stmt, 1, user->getBalance());
    sqlite3_bind_int(stmt, 2, user->getId());
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool UserManager::updatePassword(User *user)
{
    const char *sql = "UPDATE users SET password = ? WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, user->getPassword().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user->getId());
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}
