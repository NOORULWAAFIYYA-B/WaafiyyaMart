#include "database/Database.h"

#include <iostream>

Database::Database(const std::string& dbPath) : dbPath_(dbPath) {}

Database::~Database() {
    close();
}

bool Database::open() {
    if (db_ != nullptr) {
        return true;
    }

    if (sqlite3_open(dbPath_.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
        close();
        return false;
    }

    return true;
}

void Database::close() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::execute(const std::string& sql) {
    if (db_ == nullptr) {
        return false;
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << (errMsg ? errMsg : "unknown") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

sqlite3* Database::connection() const {
    return db_;
}
