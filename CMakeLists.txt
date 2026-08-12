cmake_minimum_required(VERSION 3.16)
project(WaafiyyaMart LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)

add_executable(waafiyyamart
    src/main.cpp
    src/database/Database.cpp
    src/repositories/UserRepository.cpp
    src/controllers/AuthController.cpp
)

target_include_directories(waafiyyamart PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(waafiyyamart PRIVATE SQLite::SQLite3)
