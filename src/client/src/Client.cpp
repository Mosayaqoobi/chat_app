//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#include "Client.h"
#include "chat/Constants.h"

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

void Client::disconnect() {
    connected = false;
    if (clientSocket != -1) {
        shutdown(clientSocket, SHUT_RDWR);
        close(clientSocket);
        clientSocket = -1;
    }
}



void Client::connectToServer() {
    if (isConnected()) {
        std::cout << "Client already Connected\n";
        return;
    }
    // Create Socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        std::cerr << "Error: Failed to create Client Socket\n";
        return;
    }

    sockaddr_in serverAddress {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);


    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddress.sin_addr) != 1) {
        std::cerr << "Invalid server Address\n";
        disconnect();
        return;
    }

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Error: Failed to Connect to Server\n";
        disconnect();
        return;
    }
    connected = true;
}

bool Client::sendMessage(const std::string &message) const {
    if (!isConnected() || clientSocket == -1) {
        std::cerr << "Client is not connected\n";
        return false;
    } else if (message.empty()) {
        std::cerr << "Message is empty\n";
        return false;
    } else if (message.length() > chat::kMaxMessageSize) {
        std::cerr << "Message too long\n";
        return false;
    } else if (send(clientSocket, message.data(), message.size(), 0) == -1) {
        std::cerr << "Failed to send message\n";
        return false;
    }
    return true;
}
std::string Client::receiveMessage() {
    if (!isConnected() || clientSocket == -1) {
        std::cerr << "Client is not connected\n";
        return "";
    }
    char buffer[chat::kMaxMessageSize];
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

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



