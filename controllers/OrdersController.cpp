#include "OrdersController.h"

#include <drogon/drogon.h>
#include <json/json.h>

void OrdersController::createOrder(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
)
{
    try
    {
        auto json = req->getJsonObject();

        if (!json)
        {
            Json::Value error;
            error["message"] = "Invalid JSON request.";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(
                drogon::k400BadRequest
            );

            callback(response);
            return;
        }


        // -----------------------------
        // Read user ID
        // -----------------------------

        if (!(*json).isMember("user_id"))
        {
            Json::Value error;
            error["message"] = "User ID is required.";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(
                drogon::k400BadRequest
            );

            callback(response);
            return;
        }


        int userId =
            (*json)["user_id"].asInt();


        // -----------------------------
        // Read items
        // -----------------------------

        if (!(*json).isMember("items") ||
            !(*json)["items"].isArray() ||
            (*json)["items"].empty())
        {
            Json::Value error;
            error["message"] = "Cart is empty.";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(
                drogon::k400BadRequest
            );

            callback(response);
            return;
        }


        auto dbClient =
            drogon::app().getDbClient("default");


        // -----------------------------
        // Check user
        // -----------------------------

        auto userResult =
            dbClient->execSqlSync(
                "SELECT id FROM users "
                "WHERE id = $1",
                userId
            );


        if (userResult.empty())
        {
            Json::Value error;
            error["message"] = "User does not exist.";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(
                drogon::k404NotFound
            );

            callback(response);
            return;
        }


        // -----------------------------
        // Calculate total
        // -----------------------------

        double totalAmount = 0.0;


        for (const auto& item : (*json)["items"])
        {
            if (!item.isMember("product_id") ||
                !item.isMember("quantity"))
            {
                Json::Value error;
                error["message"] =
                    "Invalid cart item.";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                callback(response);
                return;
            }


            int productId =
                item["product_id"].asInt();

            int quantity =
                item["quantity"].asInt();


            if (quantity <= 0)
            {
                Json::Value error;
                error["message"] =
                    "Quantity must be greater than zero.";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                callback(response);
                return;
            }


            auto productResult =
                dbClient->execSqlSync(
                    "SELECT price, stock "
                    "FROM products "
                    "WHERE id = $1",
                    productId
                );


            if (productResult.empty())
            {
                Json::Value error;
                error["message"] =
                    "Product not found.";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k404NotFound
                );

                callback(response);
                return;
            }


            double price =
                productResult[0]["price"]
                    .as<double>();

            int stock =
                productResult[0]["stock"]
                    .as<int>();


            if (quantity > stock)
            {
                Json::Value error;
                error["message"] =
                    "Not enough stock available.";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                callback(response);
                return;
            }


            totalAmount +=
                price * quantity;
        }


        // -----------------------------
        // Create order
        // -----------------------------

        auto orderResult =
            dbClient->execSqlSync(
                "INSERT INTO orders "
                "(user_id, total_amount, status) "
                "VALUES ($1, $2, 'pending') "
                "RETURNING id, total_amount",
                userId,
                totalAmount
            );


        if (orderResult.empty())
        {
            throw std::runtime_error(
                "Failed to create order."
            );
        }


        int orderId =
            orderResult[0]["id"].as<int>();


        // -----------------------------
        // Insert order items
        // -----------------------------

        for (const auto& item : (*json)["items"])
        {
            int productId =
                item["product_id"].asInt();

            int quantity =
                item["quantity"].asInt();


            auto productResult =
                dbClient->execSqlSync(
                    "SELECT price "
                    "FROM products "
                    "WHERE id = $1",
                    productId
                );


            double price =
                productResult[0]["price"]
                    .as<double>();


            dbClient->execSqlSync(
                "INSERT INTO order_items "
                "(order_id, product_id, quantity, price_at_order) "
                "VALUES ($1, $2, $3, $4)",
                orderId,
                productId,
                quantity,
                price
            );


            // Reduce stock
            dbClient->execSqlSync(
                "UPDATE products "
                "SET stock = stock - $1 "
                "WHERE id = $2",
                quantity,
                productId
            );
        }


        // -----------------------------
        // Success response
        // -----------------------------

        Json::Value result;

        result["message"] =
            "Order placed successfully.";

        result["order"]["id"] =
            orderId;

        result["order"]["total_amount"] =
            totalAmount;


        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                result
            );

        response->setStatusCode(
            drogon::k201Created
        );

        callback(response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Order creation error: "
            << e.what();


        Json::Value error;

        error["message"] =
            "Unable to place order.";

        error["error"] =
            e.what();


        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                error
            );

        response->setStatusCode(
            drogon::k500InternalServerError
        );

        callback(response);
    }
}