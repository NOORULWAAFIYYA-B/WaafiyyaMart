#include "AdminProductController.h"

#include "../services/AuthSession.h"
#include "../services/ProductImageStorage.h"

#include <drogon/drogon.h>
#include <drogon/MultiPart.h>

#include <cmath>
#include <cstdlib>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
struct ProductInput
{
    std::string name;
    std::string description;
    std::string category;
    double price = 0;
    int stock = 0;
    std::string imageContent;
    std::string imageExtension;
    bool hasImage = false;
};

struct ProductInputError
{
    drogon::HttpStatusCode status;
    std::string message;
};

drogon::HttpResponsePtr errorResponse(drogon::HttpStatusCode status,
                                      const std::string& message)
{
    Json::Value error;
    error["message"] = message;
    auto response = drogon::HttpResponse::newHttpJsonResponse(error);
    response->setStatusCode(status);
    return response;
}

void setStringField(const Json::Value& json,
                    const std::string& key,
                    std::string& value)
{
    if (json.isMember(key) && json[key].isString())
    {
        value = json[key].asString();
    }
}

std::optional<ProductInputError> readProductInput(
    const drogon::HttpRequestPtr& request,
    ProductInput& input,
    bool requireImage)
{
    const auto& contentType = request->getHeader("content-type");
    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        drogon::MultiPartParser parser;
        if (parser.parse(request) != 0)
        {
            return ProductInputError{
                drogon::k400BadRequest, "Invalid multipart form data."};
        }

        const auto& fields = parser.getParameters();
        const auto readField = [&fields](const std::string& key) {
            const auto it = fields.find(key);
            return it == fields.end() ? std::string{} : it->second;
        };

        input.name = readField("name");
        input.description = readField("description");
        input.category = readField("category");

        const auto priceText = readField("price");
        const auto stockText = readField("stock");
        try
        {
            std::size_t consumed = 0;
            input.price = std::stod(priceText, &consumed);
            if (consumed != priceText.size())
            {
                throw std::invalid_argument("invalid price");
            }
            consumed = 0;
            const auto stockValue = std::stoll(stockText, &consumed);
            if (consumed != stockText.size() ||
                stockValue < 0 ||
                stockValue > std::numeric_limits<int>::max())
            {
                throw std::invalid_argument("invalid stock");
            }
            input.stock = static_cast<int>(stockValue);
        }
        catch (const std::exception&)
        {
            return ProductInputError{
                drogon::k400BadRequest,
                "Price and stock must be valid non-negative numbers."};
        }

        for (const auto& file : parser.getFiles())
        {
            if (file.getItemName() != "image" || input.hasImage)
            {
                return ProductInputError{
                    drogon::k400BadRequest,
                    "Upload one image using the image field."};
            }

            input.hasImage = true;
            input.imageContent.assign(file.fileData(), file.fileLength());
        }
    }
    else
    {
        const auto json = request->getJsonObject();
        if (!json)
        {
            return ProductInputError{
                drogon::k400BadRequest, "Invalid JSON or multipart form data."};
        }

        setStringField(*json, "name", input.name);
        setStringField(*json, "description", input.description);
        setStringField(*json, "category", input.category);
        if (!(*json)["price"].isNumeric() ||
            !(*json)["stock"].isInt())
        {
            return ProductInputError{
                drogon::k400BadRequest,
                "Price and stock must be valid non-negative numbers."};
        }
        input.price = (*json)["price"].asDouble();
        input.stock = (*json)["stock"].asInt();
    }

    if (input.name.empty() || input.description.empty() ||
        input.category.empty())
    {
        return ProductInputError{
            drogon::k400BadRequest,
            "Name, description, and category are required."};
    }

    if (input.name.size() > 160 || input.description.size() > 2000 ||
        input.category.size() > 100 || input.price > 99999999.99)
    {
        return ProductInputError{
            drogon::k400BadRequest,
            "Product details exceed the allowed length or price limit."};
    }

    if (!std::isfinite(input.price) || input.price < 0 || input.stock < 0)
    {
        return ProductInputError{
            drogon::k400BadRequest,
            "Price and stock must be valid non-negative numbers."};
    }

    if (!input.hasImage)
    {
        if (requireImage)
        {
            return ProductInputError{
                drogon::k400BadRequest, "A product image is required."};
        }
        return std::nullopt;
    }

    const auto validation = ProductImageStorage::validate(
        input.imageContent, input.imageExtension);
    if (validation == ProductImageStorage::ImageValidationResult::TooLarge)
    {
        return ProductInputError{
            drogon::k413RequestEntityTooLarge,
            "Product images must be 5 MB or smaller."};
    }
    if (validation == ProductImageStorage::ImageValidationResult::Missing)
    {
        return ProductInputError{
            drogon::k400BadRequest, "The selected image file is empty."};
    }
    if (validation != ProductImageStorage::ImageValidationResult::Valid)
    {
        return ProductInputError{
            drogon::k415UnsupportedMediaType,
            "Upload a valid JPEG, PNG, or WebP image."};
    }

    return std::nullopt;
}

Json::Value toJsonProduct(const drogon::orm::Row& row)
{
    Json::Value product;
    product["id"] = row["id"].as<int>();
    product["name"] = row["name"].as<std::string>();
    product["description"] = row["description"].as<std::string>();
    product["price"] = row["price"].as<double>();
    product["stock"] = row["stock"].as<int>();
    product["category"] = row["category"].as<std::string>();
    product["image_url"] = row["image_url"].isNull()
                                ? ""
                                : row["image_url"].as<std::string>();
    return product;
}

std::optional<int> authorizeSeller(
    const drogon::HttpRequestPtr& request,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback)
{
    const auto user = AuthSession::authenticate(request);
    if (!user)
    {
        callback(errorResponse(drogon::k401Unauthorized,
                               "Sign in to manage products."));
        return std::nullopt;
    }
    if (user->role != "seller")
    {
        callback(errorResponse(drogon::k403Forbidden,
                               "A seller account is required."));
        return std::nullopt;
    }
    return user->id;
}
}

void AdminProductController::addProduct(
    const HttpRequestPtr& request,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    std::optional<ProductImageStorage::SavedImage> savedImage;
    try
    {
        const auto sellerId = authorizeSeller(request, callback);
        if (!sellerId)
        {
            return;
        }

        ProductInput input;
        if (const auto error = readProductInput(request, input, true))
        {
            callback(errorResponse(error->status, error->message));
            return;
        }

        savedImage = ProductImageStorage::save(
            input.imageContent, input.imageExtension);

        auto dbClient = drogon::app().getDbClient("default");
        const auto result = dbClient->execSqlSync(
            "INSERT INTO products "
            "(name, description, price, image_url, stock, category, seller_id) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7) "
            "RETURNING id, name, description, price, image_url, stock, category",
            input.name,
            input.description,
            input.price,
            savedImage->url,
            input.stock,
            input.category,
            *sellerId);

        Json::Value responseJson;
        responseJson["message"] = "Product added successfully.";
        responseJson["product"] = toJsonProduct(result[0]);
        auto response = HttpResponse::newHttpJsonResponse(responseJson);
        response->setStatusCode(k201Created);
        callback(response);
    }
    catch (const ProductImageStorage::StorageError& error)
    {
        if (savedImage)
        {
            ProductImageStorage::remove(savedImage->path);
        }
        LOG_ERROR << "Product image storage failure: " << error.what();
        callback(errorResponse(k503ServiceUnavailable,
                               "Product image storage is unavailable. "
                               "Configure persistent storage and try again."));
    }
    catch (const std::exception& error)
    {
        if (savedImage)
        {
            ProductImageStorage::remove(savedImage->path);
        }
        LOG_ERROR << "Add product error: " << error.what();
        callback(errorResponse(k500InternalServerError,
                               "Unable to add product."));
    }
}

void AdminProductController::getProducts(
    const HttpRequestPtr& request,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        const auto sellerId = authorizeSeller(request, callback);
        if (!sellerId)
        {
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");
        const auto result = dbClient->execSqlSync(
            "SELECT id, name, description, price, image_url, stock, category "
            "FROM products WHERE seller_id = $1 ORDER BY id",
            *sellerId);

        Json::Value products(Json::arrayValue);
        for (const auto& row : result)
        {
            products.append(toJsonProduct(row));
        }

        Json::Value responseJson;
        responseJson["products"] = products;
        callback(HttpResponse::newHttpJsonResponse(responseJson));
    }
    catch (const std::exception& error)
    {
        LOG_ERROR << "Get seller products error: " << error.what();
        callback(errorResponse(k500InternalServerError,
                               "Unable to fetch your products."));
    }
}

void AdminProductController::updateProduct(
    const HttpRequestPtr& request,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int productId)
{
    std::optional<ProductImageStorage::SavedImage> savedImage;
    try
    {
        const auto sellerId = authorizeSeller(request, callback);
        if (!sellerId)
        {
            return;
        }

        ProductInput input;
        if (const auto error = readProductInput(request, input, false))
        {
            callback(errorResponse(error->status, error->message));
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");
        const auto existing = dbClient->execSqlSync(
            "SELECT image_url FROM products "
            "WHERE id = $1 AND seller_id = $2",
            productId,
            *sellerId);
        if (existing.empty())
        {
            callback(errorResponse(k404NotFound, "Product not found."));
            return;
        }

        std::string imageUrl = existing[0]["image_url"].isNull()
                                   ? ""
                                   : existing[0]["image_url"]
                                         .as<std::string>();
        if (input.hasImage)
        {
            savedImage = ProductImageStorage::save(
                input.imageContent, input.imageExtension);
            imageUrl = savedImage->url;
        }

        const auto result = dbClient->execSqlSync(
            "UPDATE products "
            "SET name = $1, description = $2, price = $3, image_url = $4, "
            "stock = $5, category = $6 "
            "WHERE id = $7 AND seller_id = $8 "
            "RETURNING id, name, description, price, image_url, stock, category",
            input.name,
            input.description,
            input.price,
            imageUrl,
            input.stock,
            input.category,
            productId,
            *sellerId);
        if (result.empty())
        {
            if (savedImage)
            {
                ProductImageStorage::remove(savedImage->path);
            }
            callback(errorResponse(k404NotFound, "Product not found."));
            return;
        }

        Json::Value responseJson;
        responseJson["message"] = "Product updated successfully.";
        responseJson["product"] = toJsonProduct(result[0]);
        callback(HttpResponse::newHttpJsonResponse(responseJson));
    }
    catch (const ProductImageStorage::StorageError& error)
    {
        if (savedImage)
        {
            ProductImageStorage::remove(savedImage->path);
        }
        LOG_ERROR << "Product image storage failure: " << error.what();
        callback(errorResponse(k503ServiceUnavailable,
                               "Product image storage is unavailable. "
                               "Configure persistent storage and try again."));
    }
    catch (const std::exception& error)
    {
        if (savedImage)
        {
            ProductImageStorage::remove(savedImage->path);
        }
        LOG_ERROR << "Update product error: " << error.what();
        callback(errorResponse(k500InternalServerError,
                               "Unable to update product."));
    }
}

void AdminProductController::deleteProduct(
    const HttpRequestPtr& request,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int productId)
{
    try
    {
        const auto sellerId = authorizeSeller(request, callback);
        if (!sellerId)
        {
            return;
        }

        auto dbClient = drogon::app().getDbClient("default");
        const auto result = dbClient->execSqlSync(
            "DELETE FROM products "
            "WHERE id = $1 AND seller_id = $2 "
            "RETURNING id",
            productId,
            *sellerId);
        if (result.empty())
        {
            callback(errorResponse(k404NotFound, "Product not found."));
            return;
        }

        Json::Value responseJson;
        responseJson["message"] = "Product deleted successfully.";
        responseJson["product_id"] = result[0]["id"].as<int>();
        callback(HttpResponse::newHttpJsonResponse(responseJson));
    }
    catch (const std::exception& error)
    {
        LOG_ERROR << "Delete product error: " << error.what();
        callback(errorResponse(k500InternalServerError,
                               "Unable to delete product."));
    }
}
