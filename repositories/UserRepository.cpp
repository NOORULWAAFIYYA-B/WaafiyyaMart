#include "UserRepository.h"

#include <drogon/drogon.h>

UserRepository::UserRepository()
{
}

bool UserRepository::createUser(
    const std::string& fullName,
    const std::string& email,
    const std::string& passwordHash
)
{
    try
    {
        auto dbClient = drogon::app().getDbClient("default");

        dbClient->execSqlSync(
            "INSERT INTO users (full_name, email, password_hash) "
            "VALUES ($1, $2, $3)",
            fullName,
            email,
            passwordHash
        );

        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Create user error: " << e.what();
        return false;
    }
}

bool UserRepository::emailExists(const std::string& email)
{
    try
    {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT id FROM users WHERE email = $1 LIMIT 1",
            email
        );

        return result.size() > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Check email error: " << e.what();
        return false;
    }
}