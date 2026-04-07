#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <functional>
#include <sqlite3.h>

class Database
{
public:
    explicit Database(const std::string &dbPath);
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    sqlite3 *handle() const { return _db; }

private:
    sqlite3 *_db;
};

#endif
