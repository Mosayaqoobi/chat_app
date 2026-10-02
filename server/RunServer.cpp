//
// Created by Mosa Yaqoobi on 2026-06-07.
//
// contains the main for starting the server

#include "Server.h"
#include "chat/Constants.h"

#include <iostream>

int main() {
    constexpr auto kDefaultPort = 8080;

    Server server{kDefaultPort};
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
    server.stop("Server has disconnected, quitting you out");
    return 0;

}

