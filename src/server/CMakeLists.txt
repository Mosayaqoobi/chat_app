add_library(chat_server_lib
        src/Server.cpp
)

target_include_directories(chat_server_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(chat_server_lib
        PUBLIC chat_common
        PRIVATE Threads::Threads
)

add_executable(chat_server
        apps/main.cpp
)

target_link_libraries(chat_server
        PRIVATE chat_server_lib
)

add_subdirectory(tests)