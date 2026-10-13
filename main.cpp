#include <drogon/drogon.h>

#include <array>
#include <atomic>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

namespace
{
using DatabaseReady = std::shared_ptr<std::atomic<bool>>;

constexpr std::array<const char*, 4> kStartupMigrations = {
    "ALTER TABLE products "
    "ADD COLUMN IF NOT EXISTS seller_id INTEGER REFERENCES users(id)",
    "CREATE TABLE IF NOT EXISTS user_sessions ("
    "token_hash CHAR(64) PRIMARY KEY, "
    "user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE, "
    "expires_at TIMESTAMP NOT NULL, "
    "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)",
    "CREATE INDEX IF NOT EXISTS idx_products_seller_id "
    "ON products(seller_id)",
    "CREATE INDEX IF NOT EXISTS idx_user_sessions_user_id "
    "ON user_sessions(user_id)"};

void runStartupMigration(const drogon::orm::DbClientPtr& dbClient,
                         std::size_t index,
                         const DatabaseReady& databaseReady)
{
    if (index == kStartupMigrations.size())
    {
        databaseReady->store(true, std::memory_order_release);
        LOG_INFO << "Startup database migrations completed";
        return;
    }

    LOG_INFO << "Running startup database migration "
             << (index + 1) << "/" << kStartupMigrations.size();
    dbClient->execSqlAsync(
        kStartupMigrations[index],
        [dbClient, index, databaseReady](const drogon::orm::Result&) {
            runStartupMigration(dbClient, index + 1, databaseReady);
        },
        [databaseReady, index](
            const drogon::orm::DrogonDbException& error) {
            LOG_ERROR << "Startup database migration " << (index + 1)
                      << " failed: " << error.base().what();
            databaseReady->store(false, std::memory_order_release);
            drogon::app().quit();
        });
}
}

int main()
{
    const char* portEnvironment = std::getenv("PORT");
    const std::string port =
        portEnvironment && *portEnvironment ? portEnvironment : "8080";
    std::cout << "WaafiyyaMart process starting; configured PORT="
              << port << std::endl;

    try
    {
        auto& app = drogon::app();
        std::cerr << "Loading Drogon configuration" << std::endl;
        app.loadConfigFile("./config/config.json");
        std::cerr << "Drogon configuration loaded" << std::endl;

        app.setDocumentRoot("./public")
            .setClientMaxBodySize(6 * 1024 * 1024);

        if (std::getenv("RENDER") && !std::getenv("PRODUCT_IMAGE_DIR"))
        {
            LOG_WARN << "PRODUCT_IMAGE_DIR is unset; product image uploads "
                        "will be unavailable until persistent storage is "
                        "configured";
        }
        else if (std::getenv("PRODUCT_IMAGE_DIR"))
        {
            LOG_INFO << "Product image storage directory is configured";
        }

        const auto databaseReady = std::make_shared<std::atomic<bool>>(false);
        app.registerPreRoutingAdvice(
            [databaseReady](const drogon::HttpRequestPtr&,
                            drogon::AdviceCallback&& callback,
                            drogon::AdviceChainCallback&& chainCallback) {
                if (!databaseReady->load(std::memory_order_acquire))
                {
                    Json::Value body;
                    body["message"] = "Service initialization in progress.";
                    auto response =
                        drogon::HttpResponse::newHttpJsonResponse(body);
                    response->setStatusCode(drogon::k503ServiceUnavailable);
                    callback(response);
                    return;
                }

                chainCallback();
            });
        app.registerBeginningAdvice([databaseReady]() {
            LOG_INFO << "Drogon event loop started; initializing database";
            try
            {
                auto dbClient = drogon::app().getDbClient("default");
                if (!dbClient)
                {
                    LOG_ERROR << "Configured PostgreSQL client is unavailable";
                    drogon::app().quit();
                    return;
                }

                runStartupMigration(dbClient, 0, databaseReady);
            }
            catch (const std::exception& error)
            {
                LOG_ERROR << "Unable to initialize PostgreSQL client: "
                          << error.what();
                drogon::app().quit();
            }
        });

        std::cerr << "Starting Drogon with listener from config/config.json"
                  << std::endl;
        app.run();
        std::cerr << "Drogon event loop stopped" << std::endl;
    }
    catch (const std::exception& error)
    {
        std::cerr << "WaafiyyaMart startup failed: "
                  << error.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "WaafiyyaMart startup failed with an unknown error"
                  << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
