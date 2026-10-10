#include "AdminProductController.h"

#include <drogon/drogon.h>

void AdminProductController::addProduct(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
)
{
    try
    {
        auto json = req->getJsonObject();

        if (!json)
        {
            Json::Value error;
            error["message"] = "Invalid JSON.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        std::string name =
            (*json)["name"].asString();

        std::string description =
            (*json)["description"].asString();

        double price =
            (*json)["price"].asDouble();

        int stock =
            (*json)["stock"].asInt();

        std::string category =
            (*json)["category"].asString();

        if (name.empty() ||
            description.empty() ||
            category.empty())
        {
            Json::Value error;
            error["message"] =
                "All product fields are required.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        if (price < 0 || stock < 0)
        {
            Json::Value error;
            error["message"] =
                "Price and stock cannot be negative.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        auto dbClient =
            drogon::app().getDbClient("default");

        auto result =
            dbClient->execSqlSync(
                "INSERT INTO products "
                "(name, description, price, stock, category) "
                "VALUES ($1, $2, $3, $4, $5) "
                "RETURNING id, name, description, price, stock, category",
                name,
                description,
                price,
                stock,
                category
            );

        Json::Value product;

        product["id"] =
            result[0]["id"].as<int>();

        product["name"] =
            result[0]["name"].as<std::string>();

        product["description"] =
            result[0]["description"].as<std::string>();

        product["price"] =
            std::stod(
                result[0]["price"].as<std::string>()
            );

        product["stock"] =
            result[0]["stock"].as<int>();

        product["category"] =
            result[0]["category"].as<std::string>();

        Json::Value responseJson;

        responseJson["message"] =
            "Product added successfully.";

        responseJson["product"] =
            product;

        auto response =
            HttpResponse::newHttpJsonResponse(
                responseJson
            );

        response->setStatusCode(k201Created);

        callback(response);
    }
    catch (const std::exception& error)
    {
        LOG_ERROR
            << "Add product error: "
            << error.what();

        Json::Value responseJson;

        responseJson["message"] =
            "Unable to add product.";

        auto response =
            HttpResponse::newHttpJsonResponse(
                responseJson
            );

        response->setStatusCode(
            k500InternalServerError
        );

        callback(response);
    }
}


void AdminProductController::getProducts(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
)
{
    try
    {
        auto dbClient =
            drogon::app().getDbClient("default");

        auto result =
            dbClient->execSqlSync(
                "SELECT id, name, description, price, "
                "stock, category "
                "FROM products "
                "ORDER BY id"
            );

        Json::Value products(Json::arrayValue);

        for (const auto& row : result)
        {
            Json::Value product;

            product["id"] =
                row["id"].as<int>();

            product["name"] =
                row["name"].as<std::string>();

            product["description"] =
                row["description"].as<std::string>();

            product["price"] =
                std::stod(
                    row["price"].as<std::string>()
                );

            product["stock"] =
                row["stock"].as<int>();

            product["category"] =
                row["category"].as<std::string>();

            products.append(product);
        }

        Json::Value responseJson;

        responseJson["products"] =
            products;

        auto response =
            HttpResponse::newHttpJsonResponse(
                responseJson
            );

        response->setStatusCode(k200OK);

        callback(response);
    }
    catch (const std::exception& error)
    {
        LOG_ERROR
            << "Get products error: "
            << error.what();

        Json::Value responseJson;

        responseJson["message"] =
            "Unable to fetch products.";

        auto response =
            HttpResponse::newHttpJsonResponse(
                responseJson
            );

        response->setStatusCode(
            k500InternalServerError
        );

        callback(response);
    }
}


void AdminProductController::updateProduct(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int productId
)
{
    try
    {
        auto json = req->getJsonObject();

        if (!json)
        {
            Json::Value error;
            error["message"] = "Invalid JSON.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        std::string name =
            (*json)["name"].asString();

        std::string description =
            (*json)["description"].asString();

        double price =
            (*json)["price"].asDouble();

        int stock =
            (*json)["stock"].asInt();

        std::string category =
            (*json)["category"].asString();

        if (name.empty() ||
            description.empty() ||
            category.empty())
        {
            Json::Value error;
            error["message"] =
                "All product fields are required.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        if (price < 0 || stock < 0)
        {
            Json::Value error;
            error["message"] =
                "Price and stock cannot be negative.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        auto dbClient =
            drogon::app().getDbClient("default");

        auto result =
            dbClient->execSqlSync(
                "UPDATE products "
                "SET name = $1, "
                "description = $2, "
                "price = $3, "
                "stock = $4, "
                "category = $5 "
                "WHERE id = $6 "
                "RETURNING id, name, description, price, stock, category",
                name,
                description,
                price,
                stock,
                category,
                productId
            );

        if (result.size() == 0)
        {
            Json::Value error;
            error["message"] =
                "Product not found.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k404NotFound);
            callback(response);
            return;
        }

        Json::Value product;

        product["id"] =
            result[0]["id"].as<int>();

        product["name"] =
            result[0]["name"].as<std::string>();

        product["description"] =
            result[0]["description"].as<std::string>();

        product["price"] =
            std::stod(
                result[0]["price"].as<std::string>()
            );

        product["stock"] =
            result[0]["stock"].as<int>();

        product["category"] =
            result[0]["category"].as<std::string>();

        Json::Value responseJson;

        responseJson["message"] =
            "Product updated successfully.";

        responseJson["product"] =
            product;

        auto response =
            HttpResponse::newHttpJsonResponse(
                responseJson
            );

        response->setStatusCode(k200OK);

        callback(response);
    }
    catch (const std::exception& error)
    {
        LOG_ERROR
            << "Update product error: "
            << error.what();

        Json::Value responseJson;

        responseJson["message"] =
            "Unable to update product.";

        auto response =
            HttpResponse::newHttpJsonResponse(
                responseJson
            );

        response->setStatusCode(
            k500InternalServerError
        );

        callback(response);
    }
}


void AdminProductController::deleteProduct(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int productId
)
{
    try
    {
        auto dbClient =
            drogon::app().getDbClient("default");

        auto result =
            dbClient->execSqlSync(
                "DELETE FROM products "
                "WHERE id = $1 "
                "RETURNING id, name",
                productId
            );

        if (result.size() == 0)
        {
            Json::Value error;
            error["message"] =
                "Product not found.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k404NotFound);
            callback(response);
            return;
        }

        Json::Value responseJson;

        responseJson["message"] =
            "Product deleted successfully.";

        responseJson["product_id"] =
            result[0]["id"].as<int>();

        responseJson["product_name"] =
            result[0]["name"].as<std::string>();

        auto response =
            HttpResponse::newHttpJsonResponse(
                responseJson
            );

        response->setStatusCode(k200OK);

        callback(response);
    }
    catch (const std::exception& error)
    {
        LOG_ERROR
            << "Delete product error: "
            << error.what();

        Json::Value responseJson;

        responseJson["message"] =
            "Unable to delete product.";

        auto response =
            HttpResponse::newHttpJsonResponse(
                responseJson
            );

        response->setStatusCode(
            k500InternalServerError
        );

        callback(response);
    }
}