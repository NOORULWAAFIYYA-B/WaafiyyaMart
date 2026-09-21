#include "OrderHistoryController.h"

#include <drogon/drogon.h>

#include <cstdlib>
#include <string>


void OrderHistoryController::getOrderHistory(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
)
{
    try
    {
        // --------------------------------------------------
        // GET USER ID FROM QUERY STRING
        // Example:
        // /api/orders?user_id=1
        // --------------------------------------------------

        const std::string userIdText =
            req->getParameter("user_id");


        if (userIdText.empty())
        {
            Json::Value error;
            error["message"] =
                "User ID is required.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k400BadRequest);

            callback(response);
            return;
        }


        char* endPtr = nullptr;

        long userId =
            std::strtol(
                userIdText.c_str(),
                &endPtr,
                10
            );


        if (
            endPtr == userIdText.c_str() ||
            *endPtr != '\0' ||
            userId <= 0
        )
        {
            Json::Value error;
            error["message"] =
                "Invalid user ID.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k400BadRequest);

            callback(response);
            return;
        }


        // --------------------------------------------------
        // DATABASE CONNECTION
        // --------------------------------------------------

        auto dbClient =
            drogon::app().getDbClient("default");


        // --------------------------------------------------
        // CHECK USER EXISTS
        // --------------------------------------------------

        auto userResult =
            dbClient->execSqlSync(
                "SELECT id "
                "FROM users "
                "WHERE id = $1",
                static_cast<int>(userId)
            );


        if (userResult.empty())
        {
            Json::Value error;
            error["message"] =
                "User not found.";

            auto response =
                HttpResponse::newHttpJsonResponse(error);

            response->setStatusCode(k404NotFound);

            callback(response);
            return;
        }


        // --------------------------------------------------
        // GET ORDERS + ORDER ITEMS + PRODUCT NAMES
        // --------------------------------------------------

        auto result =
            dbClient->execSqlSync(
                "SELECT "
                "o.id AS order_id, "
                "o.total_amount::text AS total_amount, "
                "o.status, "
                "o.created_at::text AS created_at, "
                "oi.id AS item_id, "
                "oi.product_id, "
                "p.name AS product_name, "
                "oi.quantity, "
                "oi.price_at_order::text AS price_at_order "
                "FROM orders o "
                "LEFT JOIN order_items oi "
                "ON oi.order_id = o.id "
                "LEFT JOIN products p "
                "ON p.id = oi.product_id "
                "WHERE o.user_id = $1 "
                "ORDER BY "
                "o.created_at DESC, "
                "o.id DESC, "
                "oi.id ASC",
                static_cast<int>(userId)
            );


        // --------------------------------------------------
        // BUILD JSON RESPONSE
        // --------------------------------------------------

        Json::Value orders(Json::arrayValue);

        int currentOrderId = -1;
        int currentOrderIndex = -1;


        for (const auto& row : result)
        {
            const int orderId =
                row["order_id"].as<int>();


            // --------------------------------------------------
            // NEW ORDER
            // --------------------------------------------------

            if (orderId != currentOrderId)
            {
                Json::Value order;

                order["id"] = orderId;

                order["total_amount"] =
                    std::stod(
                        row["total_amount"].as<std::string>()
                    );

                order["status"] =
                    row["status"].as<std::string>();

                order["created_at"] =
                    row["created_at"].as<std::string>();

                order["items"] =
                    Json::Value(Json::arrayValue);


                orders.append(order);


                currentOrderIndex =
                    static_cast<int>(orders.size()) - 1;

                currentOrderId =
                    orderId;
            }


            // --------------------------------------------------
            // ORDER ITEM
            // --------------------------------------------------

            if (
                !row["item_id"].isNull() &&
                currentOrderIndex >= 0
            )
            {
                Json::Value item;

                item["id"] =
                    row["item_id"].as<int>();

                item["product_id"] =
                    row["product_id"].as<int>();

                item["name"] =
                    row["product_name"].as<std::string>();

                item["quantity"] =
                    row["quantity"].as<int>();

                item["price_at_order"] =
                    std::stod(
                        row["price_at_order"].as<std::string>()
                    );


                orders[currentOrderIndex]["items"].append(item);
            }
        }


        // --------------------------------------------------
        // FINAL RESPONSE
        // --------------------------------------------------

        Json::Value responseJson;

        responseJson["orders"] = orders;


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
            << "Order history error: "
            << error.what();


        Json::Value responseJson;

        responseJson["message"] =
            "Unable to load order history.";


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