cmake_minimum_required(VERSION 4.2)
project(chat_app)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)
find_package(Threads REQUIRED)

add_executable(chat_server
        apps/chat_server.cpp
        src/Server.cpp
)
add_executable(chat_client
        apps/chat_client.cpp
        src/Client.cpp
)

target_link_libraries(chat_server PRIVATE Threads::Threads)
target_link_libraries(chat_client PRIVATE Threads::Threads)
