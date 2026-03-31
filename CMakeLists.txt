cmake_minimum_required(VERSION 3.10)
project(shopping_platform)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(shopping_platform
    src/main.cpp
    src/User.cpp
    src/Commodity.cpp
)

target_include_directories(shopping_platform PRIVATE include)

target_compile_options(shopping_platform PRIVATE -Wall -Wextra)