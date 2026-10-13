#include "AuthSession.h"

#include <drogon/drogon.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace
{
std::string randomToken()
{
    std::array<unsigned char, 32> bytes{};
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1)
    {
        throw std::runtime_error("Unable to generate an authentication token");
    }

    std::ostringstream token;
    token << std::hex << std::setfill('0');
    for (const auto byte : bytes)
    {
        token << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return token.str();
}

std::string hashToken(const std::string& token)
{
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLength = 0;
    if (EVP_Digest(token.data(),
                   token.size(),
                   digest.data(),
                   &digestLength,
                   EVP_sha256(),
                   nullptr) != 1)
    {
        throw std::runtime_error("Unable to hash an authentication token");
    }

    std::ostringstream hash;
    hash << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digestLength; ++i)
    {
        hash << std::setw(2) << static_cast<unsigned int>(digest[i]);
    }
    return hash.str();
}
}

std::string AuthSession::createSession(int userId)
{
    const auto token = randomToken();
    const auto tokenHash = hashToken(token);

    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlSync(
        "INSERT INTO user_sessions (token_hash, user_id, expires_at) "
        "VALUES ($1, $2, CURRENT_TIMESTAMP + INTERVAL '30 days')",
        tokenHash,
        userId);

    return token;
}

std::optional<AuthSession::User> AuthSession::authenticate(
    const drogon::HttpRequestPtr& request)
{
    const auto& authorization = request->getHeader("authorization");
    constexpr std::string_view prefix = "Bearer ";
    if (authorization.size() <= prefix.size() ||
        authorization.compare(0, prefix.size(), prefix) != 0)
    {
        return std::nullopt;
    }

    const auto token = authorization.substr(prefix.size());
    if (token.size() != 64)
    {
        return std::nullopt;
    }

    auto dbClient = drogon::app().getDbClient("default");
    const auto result = dbClient->execSqlSync(
        "SELECT users.id, users.role "
        "FROM user_sessions "
        "JOIN users ON users.id = user_sessions.user_id "
        "WHERE user_sessions.token_hash = $1 "
        "AND user_sessions.expires_at > CURRENT_TIMESTAMP "
        "LIMIT 1",
        hashToken(token));

    if (result.empty())
    {
        return std::nullopt;
    }

    return User{result[0]["id"].as<int>(),
                result[0]["role"].as<std::string>()};
}

void AuthSession::revoke(const drogon::HttpRequestPtr& request)
{
    const auto& authorization = request->getHeader("authorization");
    constexpr std::string_view prefix = "Bearer ";
    if (authorization.size() <= prefix.size() ||
        authorization.compare(0, prefix.size(), prefix) != 0)
    {
        return;
    }

    const auto token = authorization.substr(prefix.size());
    if (token.size() != 64)
    {
        return;
    }

    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlSync(
        "DELETE FROM user_sessions WHERE token_hash = $1",
        hashToken(token));
}
