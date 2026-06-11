//
// Created by Mosa Yaqoobi on 2026-06-07.
//
// contains the main for starting the server

#include "Server.h"
#include <iostream>

int main() {
    Server server{"127.0.0.1", 8080};
    server.start();

    if (!server.isRunning()) {
        return 1;
    }
    std::cout << "Server Running. Type /quit to stop the server \n";

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/quit") {
            break;
        }
    }
    server.stop();
    return 0;

}

