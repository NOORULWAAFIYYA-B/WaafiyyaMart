cmake_minimum_required(VERSION 3.20)

project(WaafiyyaMart VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(WaafiyyaMart

    main.cpp

    controllers/AuthController.cpp
    controllers/ProductContoller.cpp
    controllers/OrdersController.cpp
    controllers/OrderHistoryController.cpp
    controllers/AdminProductController.cpp
    controllers/ProductImageController.cpp

    repositories/UserRepository.cpp
    services/AuthSession.cpp
    services/ProductImageStorage.cpp
)

target_link_libraries(WaafiyyaMart

    PRIVATE

    Drogon::Drogon
    PostgreSQL::PostgreSQL
    OpenSSL::Crypto
)

target_include_directories(WaafiyyaMart

    PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}
)