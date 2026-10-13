#include "ProductImageController.h"

#include "../services/ProductImageStorage.h"

#include <drogon/drogon.h>

#include <fstream>
#include <iterator>

void ProductImageController::getImage(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string filename)
{
    try
    {
        std::string path;
        std::string mimeType;
        if (!ProductImageStorage::resolveUrl("/uploads/" + filename,
                                             path,
                                             mimeType))
        {
            callback(drogon::HttpResponse::newNotFoundResponse());
            return;
        }

        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            callback(drogon::HttpResponse::newNotFoundResponse());
            return;
        }

        std::string content((std::istreambuf_iterator<char>(input)),
                            std::istreambuf_iterator<char>());
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setBody(std::move(content));
        response->setContentTypeString(mimeType);
        response->addHeader("Cache-Control",
                            "public, max-age=31536000, immutable");
        callback(response);
    }
    catch (const ProductImageStorage::StorageError& error)
    {
        LOG_ERROR << "Product image storage failure: " << error.what();
        Json::Value body;
        body["message"] = "Persistent product image storage is unavailable.";
        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(drogon::k503ServiceUnavailable);
        callback(response);
    }
}
