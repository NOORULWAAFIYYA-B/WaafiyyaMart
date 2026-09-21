#pragma once

#include <string>

struct User
{
    int id;
    std::string fullName;
    std::string email;
    std::string passwordHash;
    std::string role;
};
