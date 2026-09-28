add_library(chat_server_lib
        Server.cpp
)

target_include_directories(chat_server_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(chat_server_lib
        PUBLIC chat_common
        PRIVATE Threads::Threads
)

add_executable(chat_server
        RunServer.cpp
)

target_link_libraries(chat_server
        PRIVATE chat_server_lib
)

add_executable(server_tests tests/test_server.cpp)
target_link_libraries(server_tests PRIVATE chat_server_lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(server_tests)