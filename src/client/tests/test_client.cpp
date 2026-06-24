//
// Created by Mosa Yaqoobi on 2026-06-23.
//

#include <gtest/gtest.h>
#include "Client.h"

TEST(ClientTest, SendMessageFailsWhenNotConnected) {
    Client client("alice", "127.0.0.1", 8080);

    EXPECT_FALSE(client.sendMessage("hello"));
}
