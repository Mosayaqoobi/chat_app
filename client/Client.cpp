//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#include "Client.h"

#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

void Client::disconnect() {
    connected_ = false;
    if (clientSocket_ != -1) {
        shutdown(clientSocket_, SHUT_RDWR);
        close(clientSocket_);
        clientSocket_ = -1;
    }
}



void Client::connectToServer() {
    if (isConnected()) {
        std::cout << "Client already Connected\n";
        return;
    }
    addrinfo hints{}, *res{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    const std::string port = std::to_string(server_.port);
    if (int stat {}; (stat = getaddrinfo(server_.ip.c_str(), port.c_str(), &hints, &res)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(stat) << "\n";
        return;
    }
    for (const addrinfo* p = res; p != nullptr; p = p->ai_next) {
        clientSocket_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (clientSocket_ == -1) {
            continue;
        }
        if (connect(clientSocket_, p->ai_addr, p->ai_addrlen) == -1) {
            close(clientSocket_);
            clientSocket_ = -1;
            continue;
        }
        break;
    }
    freeaddrinfo(res);

    if (clientSocket_ == -1) {
        std::cerr << "Error: Failed to Connect to Server\n";
        return;
    }
    connected_ = true;
}

bool Client::sendMessage(const std::string &message) const {
    if (!isConnected() || clientSocket_ == -1) {
        std::cerr << "Client is not connected\n";
        return false;
    } else if (message.empty()) {
        std::cerr << "Message is empty\n";
        return false;
    } else if (message.length() > chat::kMaxMessageSize) {
        std::cerr << "Message too long\n";
        return false;
    } else if (send(clientSocket_, message.data(), message.size(), 0) == -1) {
        std::cerr << "Failed to send message\n";
        return false;
    }
    return true;
}
std::string Client::receiveMessage() {
    if (!isConnected() || clientSocket_ == -1) {
        std::cerr << "Client is not connected\n";
        return "";
    }
    char buffer[chat::kMaxMessageSize];
    ssize_t bytesReceived = recv(clientSocket_, buffer, sizeof(buffer), 0);

    if (bytesReceived == -1) {
        disconnect();
        return "";
    } else if (bytesReceived == 0) {
        std::cerr << "server disconnected\n";
        disconnect();
        return "";
    } else if (bytesReceived > 0) {
        std::string message(buffer, bytesReceived);
        return message;
    }
    return "";
}



