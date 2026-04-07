cmake_minimum_required(VERSION 3.14)
project(shopping_platform)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)

# 服务端
add_executable(shopping_server
    src/server_main.cpp
    src/Server.cpp
    src/Database.cpp
    src/OrderManager.cpp
    src/User.cpp
    src/Commodity.cpp
)
target_include_directories(shopping_server PRIVATE include)
target_link_libraries(shopping_server PRIVATE SQLite::SQLite3 pthread)
target_compile_options(shopping_server PRIVATE -Wall -Wextra)

# 客户端
add_executable(shopping_client
    src/client_main.cpp
    src/Client.cpp
)
target_include_directories(shopping_client PRIVATE include)
target_compile_options(shopping_client PRIVATE -Wall -Wextra)
