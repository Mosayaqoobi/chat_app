add_library(chat_common INTERFACE)

target_include_directories(chat_common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chat_common
        INTERFACE spdlog::spdlog
)