#pragma once

#include <sqlite3.h>
#include <string>

class Database {
public:
    Database(const std::string& dbPath = "waafiyyamart.db");
    ~Database();

    bool open();
    void close();
    bool execute(const std::string& sql);
    sqlite3* connection() const;

private:
    std::string dbPath_;
    sqlite3* db_ = nullptr;
};
