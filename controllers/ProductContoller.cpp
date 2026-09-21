#include "ProductContoller.h"

#include <drogon/drogon.h>

void ProductContoller::getProducts(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    try
    {
        auto dbClient = drogon::app().getDbClient("default");

        auto result = dbClient->execSqlSync(
            "SELECT id, name, description, price, image_url, stock, category "
            "FROM products "
            "ORDER BY id"
        );

        Json::Value products(Json::arrayValue);

        for (const auto& row : result)
        {
            Json::Value product;

            product["id"] = row["id"].as<int>();
            product["name"] = row["name"].as<std::string>();
            product["description"] = row["description"].as<std::string>();
            product["price"] = row["price"].as<double>();
            product["stock"] = row["stock"].as<int>();
            product["category"] = row["category"].as<std::string>();

            if (row["image_url"].isNull())
            {
                product["image_url"] = "";
            }
            else
            {
                product["image_url"] =
                    row["image_url"].as<std::string>();
            }

            products.append(product);
        }

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(products);

        callback(response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Get products error: " << e.what();

        Json::Value error;
        error["message"] = "Failed to load products";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(error);

        response->setStatusCode(
            drogon::k500InternalServerError
        );

        callback(response);
    }
}