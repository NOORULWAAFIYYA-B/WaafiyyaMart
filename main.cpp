
#include <drogon/drogon.h>
#include <cstdlib>
#include <iostream>

int main()
{
    std::cout << "WaafiyyaMart server starting..." << std::endl;

    const char* portEnv = std::getenv("PORT");
    const int port = portEnv ? std::atoi(portEnv) : 8080;

    auto& app = drogon::app();
    app.loadConfigFile("./config/config.json");

    auto dbClient = app.getDbClient("default");
    dbClient->execSqlSync(
        "ALTER TABLE products "
        "ADD COLUMN IF NOT EXISTS seller_id INTEGER REFERENCES users(id)");
    dbClient->execSqlSync(
        "CREATE TABLE IF NOT EXISTS user_sessions ("
        "token_hash CHAR(64) PRIMARY KEY, "
        "user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE, "
        "expires_at TIMESTAMP NOT NULL, "
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)");
    dbClient->execSqlSync(
        "CREATE INDEX IF NOT EXISTS idx_products_seller_id "
        "ON products(seller_id)");
    dbClient->execSqlSync(
        "CREATE INDEX IF NOT EXISTS idx_user_sessions_user_id "
        "ON user_sessions(user_id)");

    app
        .setDocumentRoot("./public")
        .setClientMaxBodySize(6 * 1024 * 1024)
        .addListener("0.0.0.0", port)
        .run();

    return 0;
}