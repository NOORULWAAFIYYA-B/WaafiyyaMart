#pragma once

#include <drogon/HttpController.h>

class ProductContoller : public drogon::HttpController<ProductContoller>
{
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(
        ProductContoller::getProducts,
        "/api/products",
        drogon::Get
    );

    METHOD_LIST_END

    void getProducts(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );
};