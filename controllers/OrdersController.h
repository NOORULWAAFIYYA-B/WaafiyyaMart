#pragma once

#include <drogon/HttpController.h>

class OrdersController
    : public drogon::HttpController<OrdersController>
{
public:

    METHOD_LIST_BEGIN

    ADD_METHOD_TO(
        OrdersController::createOrder,
        "/api/orders",
        drogon::Post
    );

    METHOD_LIST_END


    void createOrder(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );
};