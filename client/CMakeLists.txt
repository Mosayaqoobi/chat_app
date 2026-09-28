add_library(chat_client_lib
        Client.cpp
)

target_include_directories(chat_client_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(chat_client_lib
        PUBLIC chat_common
        PRIVATE Threads::Threads
)

add_executable(chat_client
        RunClient.cpp
)

target_link_libraries(chat_client
        PRIVATE chat_client_lib
)

add_executable(client_tests tests/test_client.cpp)
target_link_libraries(client_tests PRIVATE chat_client_lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(client_tests)