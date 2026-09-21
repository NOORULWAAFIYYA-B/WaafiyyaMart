#include <drogon/drogon.h>
#include <iostream>

int main()
{
    std::cout << "WaafiyyaMart server starting..." << std::endl;

    drogon::app()
        .loadConfigFile("./config/config.json")
        .setDocumentRoot("./public")
        .addListener("127.0.0.1", 8080)
        .run();

    return 0;
}
