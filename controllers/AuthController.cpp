#include "AuthController.h"

#include "../repositories/UserRepository.h"

#include <drogon/drogon.h>

void AuthController::registerUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    if (!json)
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Invalid JSON"}}
        );

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    std::string fullName = (*json)["full_name"].asString();
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();

    if (fullName.empty() || email.empty() || password.empty())
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "All fields are required"}}
        );

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    UserRepository repository;

    if (repository.emailExists(email))
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Email already exists"}}
        );

        response->setStatusCode(drogon::k409Conflict);
        callback(response);
        return;
    }

    bool created = repository.createUser(
        fullName,
        email,
        password
    );

    if (!created)
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Failed to create user"}}
        );

        response->setStatusCode(drogon::k500InternalServerError);
        callback(response);
        return;
    }

    Json::Value result;
    result["message"] = "Registration successful";

    auto response = drogon::HttpResponse::newHttpJsonResponse(result);

    response->setStatusCode(drogon::k201Created);
    callback(response);
}


void AuthController::loginUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    if (!json)
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Invalid JSON"}}
        );

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();

    if (email.empty() || password.empty())
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Email and password are required"}}
        );

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    try
    {
        auto result = dbClient->execSqlSync(
            "SELECT id, full_name, email, password_hash, role "
            "FROM users WHERE email = $1 LIMIT 1",
            email
        );

        if (result.empty())
        {
            auto response = drogon::HttpResponse::newHttpJsonResponse(
                Json::Value{{"message", "Invalid email or password"}}
            );

            response->setStatusCode(drogon::k401Unauthorized);
            callback(response);
            return;
        }

        std::string storedPassword =
            result[0]["password_hash"].as<std::string>();

        if (storedPassword != password)
        {
            auto response = drogon::HttpResponse::newHttpJsonResponse(
                Json::Value{{"message", "Invalid email or password"}}
            );

            response->setStatusCode(drogon::k401Unauthorized);
            callback(response);
            return;
        }

        Json::Value user;
        user["id"] = result[0]["id"].as<int>();
        user["full_name"] = result[0]["full_name"].as<std::string>();
        user["email"] = result[0]["email"].as<std::string>();
        user["role"] = result[0]["role"].as<std::string>();

        Json::Value responseData;
        responseData["message"] = "Login successful";
        responseData["user"] = user;

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(responseData);

        callback(response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Login database error: " << e.what();

        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Database error"}}
        );

        response->setStatusCode(drogon::k500InternalServerError);
        callback(response);
    }
}