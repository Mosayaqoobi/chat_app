add_library(chat_client_lib
        src/Client.cpp
)

target_include_directories(chat_client_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chat_client_lib
        PUBLIC chat_common
        PRIVATE Threads::Threads
)

add_executable(chat_client
        apps/main.cpp
)

target_link_libraries(chat_client
        PRIVATE chat_client_lib
)

add_subdirectory(tests)