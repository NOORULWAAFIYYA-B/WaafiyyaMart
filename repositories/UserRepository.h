#pragma once

#include "../models/User.h"
#include <memory>
#include <string>

class UserRepository
{
public:
    UserRepository();

    bool createUser(
        const std::string& fullName,
        const std::string& email,
        const std::string& passwordHash
    );

    bool emailExists(const std::string& email);
};
