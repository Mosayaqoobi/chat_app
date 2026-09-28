add_library(chat_common INTERFACE
        chat/Logging.h
        chat/Message.h
        chat/Endpoint.h
        chat/Constants.h
)

target_include_directories(chat_common INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(chat_common
        INTERFACE spdlog::spdlog
)