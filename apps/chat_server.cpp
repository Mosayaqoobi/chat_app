//
// Created by Mosa Yaqoobi on 2026-06-07.
//
// contains the main for starting the server

#include "Server.h"
#include "Constants.h"

#include <iostream>

int main() {
    constexpr auto kDefaultHost = "127.0.0.1";
    constexpr auto kDefaultPort = 8080;

    Server server{kDefaultHost, kDefaultPort};
    server.start();

    if (!server.isRunning()) {
        return 1;
    }
    std::cout << "Server Running. Type /quit to stop the server \n";

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == chat::kQuitCommand) {
            break;
        }
    }
    server.stop();
    return 0;

}

