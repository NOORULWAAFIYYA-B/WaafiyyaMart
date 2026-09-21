#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class OrderHistoryController
    : public drogon::HttpController<OrderHistoryController>
{
public:

    METHOD_LIST_BEGIN

        ADD_METHOD_TO(
            OrderHistoryController::getOrderHistory,
            "/api/orders",
            Get
        );

    METHOD_LIST_END


    void getOrderHistory(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );
};