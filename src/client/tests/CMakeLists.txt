add_executable(client_tests
        test_client.cpp
)

target_link_libraries(client_tests
        PRIVATE chat_client_lib GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(client_tests)