#pragma once

#include <drogon/HttpController.h>

class ProductImageController
    : public drogon::HttpController<ProductImageController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProductImageController::getImage,
                  "/uploads/{1}",
                  drogon::Get);
    METHOD_LIST_END

    void getImage(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        std::string filename);
};
