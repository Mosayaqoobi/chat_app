//
// Created by Mosa Yaqoobi on 2026-06-23.
//

#include <gtest/gtest.h>
#include "Client.h"

TEST(ClientTest, SendMessageFailsWhenNotConnected) {
    const Client client("alice", {.ip = "127.0.0.1", .port = 8080});

    EXPECT_FALSE(client.sendMessage("hello"));
}
