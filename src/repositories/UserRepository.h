#pragma once

#include <string>
#include <vector>

#include "database/Database.h"
#include "models/User.h"

class UserRepository {
public:
    explicit UserRepository(Database& db);

    bool createUser(const std::string& fullName, const std::string& email, const std::string& passwordHash);
    bool userExists(const std::string& email) const;
    std::string getPasswordHashByEmail(const std::string& email) const;
    User getUserByEmail(const std::string& email) const;
    std::vector<User> getAllUsers() const;

private:
    Database& db_;
};
