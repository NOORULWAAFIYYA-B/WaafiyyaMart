#pragma once

#include <drogon/HttpRequest.h>

#include <optional>
#include <string>

namespace AuthSession
{
struct User
{
    int id;
    std::string role;
};

std::string createSession(int userId);
std::optional<User> authenticate(const drogon::HttpRequestPtr& request);
void revoke(const drogon::HttpRequestPtr& request);
}
