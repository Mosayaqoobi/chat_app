//
// Created by Mosa Yaqoobi on 2026-06-23.
//

#include <gtest/gtest.h>
#include "Server.h"

TEST(ServerTest, AddClientRejectsDuplicate) {
    Server server(8080);

    EXPECT_TRUE(server.addClient(3));
    EXPECT_FALSE(server.addClient(3));
}
