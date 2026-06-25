cmake_minimum_required(VERSION 4.2)
project(chat_app)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

enable_testing()

add_subdirectory(src/common)
add_subdirectory(src/client)
add_subdirectory(src/server)