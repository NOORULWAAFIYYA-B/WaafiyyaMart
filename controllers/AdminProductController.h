#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class AdminProductController
    : public drogon::HttpController<AdminProductController>
{
public:

    METHOD_LIST_BEGIN

        // CREATE
        ADD_METHOD_TO(
            AdminProductController::addProduct,
            "/api/admin/products",
            Post
        );

        // READ
        ADD_METHOD_TO(
            AdminProductController::getProducts,
            "/api/admin/products",
            Get
        );

        // UPDATE
        ADD_METHOD_TO(
            AdminProductController::updateProduct,
            "/api/admin/products/{1}",
            Put
        );

        // DELETE
        ADD_METHOD_TO(
            AdminProductController::deleteProduct,
            "/api/admin/products/{1}",
            Delete
        );

    METHOD_LIST_END


    // CREATE
    void addProduct(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );


    // READ
    void getProducts(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );


    // UPDATE
    void updateProduct(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback,
        int productId
    );


    // DELETE
    void deleteProduct(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback,
        int productId
    );

};