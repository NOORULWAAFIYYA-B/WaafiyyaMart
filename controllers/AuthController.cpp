
#include "AuthController.h"
#include "../repositories/UserRepository.h"
#include "../services/AuthSession.h"
#include <drogon/drogon.h>
#include <string>

void AuthController::registerUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    if (!json)
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Invalid JSON"}});
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
            Json::Value{{"message", "All fields are required"}});
        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    UserRepository repository;

    if (repository.emailExists(email))
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Email already exists"}});
        response->setStatusCode(drogon::k409Conflict);
        callback(response);
        return;
    }

    bool created = repository.createUser(fullName, email, password);

    if (!created)
    {
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Failed to create user"}});
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
        Json::Value body;
        body["message"] = "Invalid JSON";

        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    const std::string email = (*json)["email"].asString();
    const std::string password = (*json)["password"].asString();

    if (email.empty() || password.empty())
    {
        Json::Value body;
        body["message"] = "Email and password are required";

        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    try
    {
        auto dbClient = drogon::app().getDbClient("default");

        if (!dbClient)
        {
            LOG_ERROR << "Login failed: database client unavailable";

            Json::Value body;
            body["message"] = "Database unavailable";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(body);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
            return;
        }

        auto result = dbClient->execSqlSync(
            "SELECT id, full_name, email, password_hash, role "
            "FROM users WHERE email = $1 LIMIT 1",
            email);

        if (result.empty())
        {
            Json::Value body;
            body["message"] = "Invalid email or password";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(body);
            response->setStatusCode(drogon::k401Unauthorized);
            callback(response);
            return;
        }

        const auto userId = result[0]["id"].as<int>();
        const std::string fullName =
            result[0]["full_name"].as<std::string>();
        const std::string userEmail =
            result[0]["email"].as<std::string>();
        const std::string storedPassword =
            result[0]["password_hash"].as<std::string>();
        const std::string role =
            result[0]["role"].as<std::string>();

        if (storedPassword != password)
        {
            Json::Value body;
            body["message"] = "Invalid email or password";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(body);
            response->setStatusCode(drogon::k401Unauthorized);
            callback(response);
            return;
        }

        const std::string token = AuthSession::createSession(userId);

        Json::Value user;
        user["id"] = userId;
        user["full_name"] = fullName;
        user["email"] = userEmail;
        user["role"] = role;
        user["token"] = token;

        Json::Value body;
        body["message"] = "Login successful";
        body["user"] = user;

        callback(drogon::HttpResponse::newHttpJsonResponse(body));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Login failed with exception: " << e.what();

        Json::Value body;
        body["message"] = "Login could not be completed";

        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(drogon::k500InternalServerError);
        callback(response);
    }
}


void AuthController::logoutUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        if (!AuthSession::authenticate(req))
        {
            auto response = drogon::HttpResponse::newHttpJsonResponse(
                Json::Value{{"message", "Authentication required"}});
            response->setStatusCode(drogon::k401Unauthorized);
            callback(response);
            return;
        }

        AuthSession::revoke(req);
        callback(drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Logged out successfully"}}));
    }
    catch (const std::exception& error)
    {
        LOG_ERROR << "Logout error: " << error.what();
        auto response = drogon::HttpResponse::newHttpJsonResponse(
            Json::Value{{"message", "Unable to log out"}});
        response->setStatusCode(drogon::k500InternalServerError);
        callback(response);
    }
}
