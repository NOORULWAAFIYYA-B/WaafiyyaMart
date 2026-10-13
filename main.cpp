
#include <drogon/drogon.h>
#include <cstdlib>
#include <iostream>

int main()
{
    std::cout << "WaafiyyaMart server starting..." << std::endl;

    const char* portEnv = std::getenv("PORT");
    const int port = portEnv ? std::atoi(portEnv) : 8080;

    drogon::app()
        .loadConfigFile("./config/config.json")
        .setDocumentRoot("./public")
        .addListener("0.0.0.0", port)
        .run();

    return 0;
}