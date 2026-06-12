
//
// Created by Mosa Yaqoobi on 2026-06-07.
//
//contains the main for starting a client

#include "Client.h"
#include <iostream>
#include <thread>


int main() {
    std::string username;
    std::string ip;
    std::string portStr;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    std::cout << "Enter the IP: ";
    std::getline(std::cin, ip);
    std::cout << "Enter port number: ";
    std::getline(std::cin, portStr);
    const int port = std::stoi(portStr);


    Client client {username, ip, port};

    client.connectToServer();

    if (!client.isConnected()) {
        std::cerr << "Failed to connect to server\n";
        return 1;
    }

    std::cout << "Connected. Type messages, or /quit to exit. \n";

    std::thread receiver([&client] {
        while (client.isConnected()) {
            std::string msg = client.receiveMessage();
            if (!client.isConnected()) {
                break;
            }
            if (!msg.empty()) {
                std::cout << msg << "\n";
            }
        }
    });

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/quit") {
            break;
        }
        if (line.empty()) {
            continue;
        }
        if (line.length() > 200)
        {
            std::cerr << "Message is too long (max 200 characters) \n";
            continue;
        }
        if (!client.sendMessage(line)) {
            std::cerr << "Failed to send message\n";
            break;
        }
    }
    client.disconnect();
    receiver.join();
    return 0;




}