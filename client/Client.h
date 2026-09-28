//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#pragma once

#include "chat/Endpoint.h"
#include "chat/Constants.h"

#include <atomic>
#include <string>
#include <utility>

class Client {

public:

    explicit Client(std::string username, chat::Endpoint server) :
    username_(std::move(username)), server_(std::move(server)) {}

    void connectToServer();

    [[nodiscard]] bool sendMessage(const std::string& message) const;

    std::string receiveMessage();

    [[nodiscard]] bool isConnected() const {return connected_;};

    [[nodiscard]] std::string getUsername() const { return this->username_; }

    [[nodiscard]] int getSocket() const { return this->clientSocket_; }

    void disconnect();
private:
    std::string username_ {};
    chat::Endpoint server_;
    int clientSocket_ {-1};
    std::atomic<bool> connected_ {false};

};
