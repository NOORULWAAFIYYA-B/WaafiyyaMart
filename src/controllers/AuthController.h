#pragma once

#include <string>
#include "database/Database.h"
#include "repositories/UserRepository.h"

class AuthController {
public:
    explicit AuthController(Database& db);

    bool registerUser(const std::string& fullName, const std::string& email, const std::string& password);
    bool loginUser(const std::string& email, const std::string& password);
    std::string hashPassword(const std::string& password) const;

private:
    UserRepository userRepository_;
};
