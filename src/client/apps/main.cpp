//
// Created by Mosa Yaqoobi on 2026-06-23.
//
//contains the main for starting a client

#include "Client.h"
#include "chat/Constants.h"
#include "chat/Logging.h"

#include <iostream>
#include <thread>
#include <print>


int main() {
    chat::initLogging();
    auto log = chat::clientLogger();

    std::string username;
    std::string ip;
    std::string portStr;


    std::print("Enter username: ");
    std::getline(std::cin, username);
    std::print("Enter the IP: ");
    std::getline(std::cin, ip);
    std::print("Enter port number: ");
    std::getline(std::cin, portStr);
    const int port = std::stoi(portStr);


    Client client {username, ip, port};

    client.connectToServer();

    if (!client.isConnected()) {
        log->error("Failed to connect to server");
        return 1;
    }
    std::print("Connected, Type message, or /quit to exit\n");
    log->debug("Client [{}], connected to server {}:{}", client.getSocket(), ip, port);

    std::thread receiver([&client] {
        while (client.isConnected()) {
            std::string msg = client.receiveMessage();
            if (!client.isConnected()) {
                break;
            }
            if (!msg.empty()) {
                std::cout << "\r\033[K"
                << msg << "\n"
                << ">> " << std::flush;
}
        }
    });

    std::string line;
    auto showPrompt = [] {std::print(">> "); };
    showPrompt();

    while (std::getline(std::cin, line)) {
        if (line == chat::kQuitCommand) {
            break;
        }
        if (line.empty()) {
            showPrompt();
            continue;
        }
        if (line.length() > chat::kMaxMessageSize)
        {
            std::print("Message is too long (max {} characters)\n", chat::kMaxMessageSize);
            log->warn("rejecting oversized message ({} chars)", line.length());
            showPrompt();
            continue;
        }
        if (!client.sendMessage(line)) {
            log->error("Failed to send message");
            showPrompt();
            break;
        }
        showPrompt();
    }
    client.disconnect();
    receiver.join();
    return 0;
}