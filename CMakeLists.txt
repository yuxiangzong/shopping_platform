cmake_minimum_required(VERSION 3.10)
project(shopping_platform)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 共享的业务逻辑源文件
set(COMMON_SOURCES
    src/User.cpp
    src/Commodity.cpp
    src/Order.cpp
    src/ShoppingCart.cpp
)

# 服务端
add_executable(shopping_server
    src/server_main.cpp
    src/Server.cpp
    ${COMMON_SOURCES}
)
target_include_directories(shopping_server PRIVATE include)
target_compile_options(shopping_server PRIVATE -Wall -Wextra)

# 客户端
add_executable(shopping_client
    src/client_main.cpp
    src/Client.cpp
    ${COMMON_SOURCES}
)
target_include_directories(shopping_client PRIVATE include)
target_compile_options(shopping_client PRIVATE -Wall -Wextra)
