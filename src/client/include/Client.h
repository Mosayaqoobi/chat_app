//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#pragma once

#include <atomic>
#include <string>
#include <utility>

class Client {
    std::string username {};
    std::string serverIp {};
    int serverPort {};
    int clientSocket {-1};
    std::atomic<bool> connected {false};

public:
    Client(std::string username, std::string serverIp, const int serverPort) :
    username(std::move(username)),
    serverIp(std::move(serverIp)),
    serverPort(serverPort){};

    void connectToServer();

    [[nodiscard]] bool sendMessage(const std::string& message) const;

    std::string receiveMessage();

    [[nodiscard]] bool isConnected() const {return connected;};

    [[nodiscard]] std::string getUsername() const { return this->username; }

    [[nodiscard]] int getSocket() const { return this->clientSocket; }

    void disconnect();

};
