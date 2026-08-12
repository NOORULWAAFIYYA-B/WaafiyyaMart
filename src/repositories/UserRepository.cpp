#include "repositories/UserRepository.h"

#include <iostream>
#include <sstream>

UserRepository::UserRepository(Database& db) : db_(db) {}

bool UserRepository::createUser(const std::string& fullName, const std::string& email, const std::string& passwordHash) {
    if (userExists(email)) {
        return false;
    }

    std::ostringstream sql;
    sql << "INSERT INTO users (full_name, email, password_hash, role) VALUES ('"
        << fullName << "', '" << email << "', '" << passwordHash << "', 'customer');";

    return db_.execute(sql.str());
}

bool UserRepository::userExists(const std::string& email) const {
    std::string query = "SELECT id FROM users WHERE email = '" + email + "';";
    sqlite3_stmt* stmt = nullptr;
    bool exists = false;

    if (sqlite3_prepare_v2(db_.connection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = true;
    }

    sqlite3_finalize(stmt);
    return exists;
}

std::string UserRepository::getPasswordHashByEmail(const std::string& email) const {
    std::string query = "SELECT password_hash FROM users WHERE email = '" + email + "';";
    sqlite3_stmt* stmt = nullptr;
    std::string hash;

    if (sqlite3_prepare_v2(db_.connection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return hash;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(stmt, 0);
        if (value) {
            hash = reinterpret_cast<const char*>(value);
        }
    }

    sqlite3_finalize(stmt);
    return hash;
}

User UserRepository::getUserByEmail(const std::string& email) const {
    User user;
    std::string query = "SELECT id, full_name, email, password_hash, role FROM users WHERE email = '" + email + "';";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.connection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return user;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        const unsigned char* fullName = sqlite3_column_text(stmt, 1);
        const unsigned char* emailCol = sqlite3_column_text(stmt, 2);
        const unsigned char* hashCol = sqlite3_column_text(stmt, 3);
        const unsigned char* roleCol = sqlite3_column_text(stmt, 4);

        if (fullName) user.fullName = reinterpret_cast<const char*>(fullName);
        if (emailCol) user.email = reinterpret_cast<const char*>(emailCol);
        if (hashCol) user.passwordHash = reinterpret_cast<const char*>(hashCol);
        if (roleCol) user.role = reinterpret_cast<const char*>(roleCol);
    }

    sqlite3_finalize(stmt);
    return user;
}

std::vector<User> UserRepository::getAllUsers() const {
    std::vector<User> users;
    std::string query = "SELECT id, full_name, email, password_hash, role FROM users ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.connection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return users;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(stmt, 0);
        const unsigned char* fullName = sqlite3_column_text(stmt, 1);
        const unsigned char* emailCol = sqlite3_column_text(stmt, 2);
        const unsigned char* hashCol = sqlite3_column_text(stmt, 3);
        const unsigned char* roleCol = sqlite3_column_text(stmt, 4);

        if (fullName) user.fullName = reinterpret_cast<const char*>(fullName);
        if (emailCol) user.email = reinterpret_cast<const char*>(emailCol);
        if (hashCol) user.passwordHash = reinterpret_cast<const char*>(hashCol);
        if (roleCol) user.role = reinterpret_cast<const char*>(roleCol);

        users.push_back(user);
    }

    sqlite3_finalize(stmt);
    return users;
}
