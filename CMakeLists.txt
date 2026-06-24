cmake_minimum_required(VERSION 4.2)
project(chat_app)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

enable_testing()

include(${CMAKE_SOURCE_DIR}/GTest.cmake)

add_subdirectory(src/common)
add_subdirectory(src/client)
add_subdirectory(src/server)