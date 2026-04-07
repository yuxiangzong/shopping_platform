#include <iostream>
#include <sqlite3.h>
#include "Database.h"

Database::Database(const std::string &dbPath)
{
    if (sqlite3_open(dbPath.c_str(), &_db) != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(_db) << '\n';
        sqlite3_close(_db);
        _db = nullptr;
        return;
    }

    // 启用 WAL 模式提升并发读性能
    sqlite3_exec(_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    // 启用外键约束
    sqlite3_exec(_db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

    // 建表
    const char *schema = R"(
        CREATE TABLE IF NOT EXISTS users (
            id       INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT    NOT NULL UNIQUE,
            password TEXT    NOT NULL,
            type     TEXT    NOT NULL CHECK(type IN ('Merchant','Consumer')),
            balance  REAL    NOT NULL DEFAULT 0.0
        );
        CREATE TABLE IF NOT EXISTS commodities (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT    NOT NULL UNIQUE,
            type        TEXT    NOT NULL CHECK(type IN ('Food','Clothes','Book','Electronics')),
            merchant    TEXT    NOT NULL,
            description TEXT    NOT NULL DEFAULT '',
            base_price  REAL    NOT NULL,
            discount    REAL    NOT NULL DEFAULT 1.0,
            storage     INTEGER NOT NULL DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS cart_items (
            user_id      INTEGER NOT NULL,
            commodity_id INTEGER NOT NULL,
            quantity     INTEGER NOT NULL DEFAULT 1,
            PRIMARY KEY (user_id, commodity_id)
        );
        CREATE TABLE IF NOT EXISTS orders (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id    INTEGER NOT NULL,
            status     TEXT    NOT NULL DEFAULT 'PENDING' CHECK(status IN ('PENDING','PAID','CANCELLED')),
            created_at TEXT    NOT NULL DEFAULT (datetime('now'))
        );
        CREATE TABLE IF NOT EXISTS order_items (
            order_id     INTEGER NOT NULL,
            commodity_id INTEGER NOT NULL,
            quantity     INTEGER NOT NULL,
            unit_price   REAL    NOT NULL,
            PRIMARY KEY (order_id, commodity_id)
        );
    )";

    char *errMsg = nullptr;
    if (sqlite3_exec(_db, schema, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::cerr << "Schema creation failed: " << errMsg << '\n';
        sqlite3_free(errMsg);
    }
    else
    {
        std::cout << "Database initialized: " << dbPath << '\n';
    }
}

Database::~Database()
{
    if (_db)
    {
        sqlite3_close(_db);
        std::cout << "Database closed." << '\n';
    }
}
