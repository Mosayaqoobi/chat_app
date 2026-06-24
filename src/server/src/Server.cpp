//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#include "Server.h"
#include "chat/Constants.h"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <algorithm>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/event.h>

namespace {
    constexpr int eventBatchSize = 64;  //batch size for kevents in a kqueue
    constexpr int shutdownEventIdent = 1;
    constexpr int socketOptionEnabled = 1;
}

bool Server::addClient(int clientSocket) {

    for (const auto client : clientSockets) {
        if (client == clientSocket) {
            std::cout << "[Server::addClient] Client is already connected\n";
            return false;
        }
    }
    if (clientSockets.size() >= maxClients) {
        std::cerr << "[Server::addClient] Client list is full\n";
        return false;
    }
    clientSockets.push_back(clientSocket);
    return true;
}

void Server::removeClient(const int clientSocket) {
    const auto it = std::ranges::find(clientSockets, clientSocket);
    if (it == clientSockets.end()) {
        std::cerr << "[Server::removeClient] Client not found\n";
        return;
    }
    clientSockets.erase(it);
    close(clientSocket);
    std::cout << "[Server::removeClient] Client with Socket " << clientSocket << " has been removed\n";
}

void Server::broadcastMessage(const Message& message) {
    std::vector<int> deadSockets {};
    for (const auto client : clientSockets) {
        if (client != message.senderSocket) {
            if (send(client, message.text.data(), message.text.size(), 0) == -1) {
                std::cerr << "[Server::broadcastMessage] Failed to send message\n";
                deadSockets.push_back(client);
            }
        }
    }
    for (const auto client : deadSockets) {
        removeClient(client);
    }
}

void Server::handleClient(const int clientSocket) {
    char buffer[chat::kMaxMessageSize];
    if (const ssize_t receivedBytes = recv(clientSocket, buffer, sizeof(buffer), 0); receivedBytes == -1) {
        std::cerr << "[Server::handleClient] Failed to receive message from client " <<
            clientSocket << "\n";
    } else if (receivedBytes == 0) {
        std::cerr << "[Server::handleClient] Client disconnected\n";
        removeClient(clientSocket);
    } else {
        const std::string msg(buffer, receivedBytes);
        const Message message (clientSocket, msg, Message::MessageType::Chat);
        broadcastMessage(message);
    }
}

void Server::acceptNewClient() {
    const int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket == -1) {
        std::cerr << "[Server::acceptNewClient] Failed to accept client\n";
        return;
    }
    setsockopt(clientSocket, SOL_SOCKET, SO_NOSIGPIPE, &socketOptionEnabled, sizeof(socketOptionEnabled));
    if (!addClient(clientSocket)) {
        std::cerr << "[Server::acceptNewClient] Failed to add client\n";
        close(clientSocket);
        return;
    }

    struct kevent ev{};
    EV_SET(&ev, clientSocket, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(kq, &ev, 1, nullptr, 0, nullptr) == -1) {
        std::cerr << "[Server::acceptNewClient] Failed to add event to kqueue\n";
        removeClient(clientSocket);
    }

}

void Server::eventLoop() {
    while (isRunning())
    {
        struct kevent events[eventBatchSize];

        const int n = kevent(kq, nullptr, 0, events, eventBatchSize, nullptr);

        if (n == -1) {
            std::cerr << "[Server::eventLoop] Failed to get events\n";
            continue;
        }
        if (n == 0) {
            continue;
        }
        for (int i = 0; i < n; ++i) {
            if (events[i].filter == EVFILT_USER) {
                return;
            } if (const int fd = static_cast<int>(events[i].ident); fd == serverSocket) {
                acceptNewClient();
            } else if (events[i].flags & EV_EOF) {
                removeClient(fd);   //client hung up, disconnect them
            } else {
                handleClient(fd);
            }
        }
    }

}

void Server::start() {
    if (isRunning()) {
        return;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "[Server::start] Failed to create server socket\n";
        return;
    }
    setsockopt(serverSocket, SOL_SOCKET, SO_NOSIGPIPE, &socketOptionEnabled, sizeof(socketOptionEnabled));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "[Server::start] Failed to parse ip address\n";
        close(serverSocket);
        serverSocket = -1;
        return;
    }
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
    {
        std::cerr << "[Server::start] Failed to bind server socket\n";
        close(serverSocket);
        serverSocket = -1;
        return;
    }
    if (listen(serverSocket, SOMAXCONN) == -1)
    {
        std::cerr << "[Server::start] Failed to listen on server socket\n";
        close(serverSocket);
        serverSocket = -1;
        return;
    }
    kq = kqueue();
    if (kq == -1) {
        std::cerr << "[Server::start] Failed to create kqueue\n";
        close(serverSocket);
        serverSocket = -1;
        return;
    }

    struct kevent ev{};
    EV_SET(&ev, serverSocket, EVFILT_READ, EV_ADD, 0, 0, nullptr);

    if (kevent(kq, &ev, 1, nullptr, 0, nullptr) == -1) {
        std::cerr << "[Server::start] Failed to add event to kqueue\n";
        close(serverSocket);
        serverSocket = -1;
        close(kq);
        kq = -1;
        return;
    }
    EV_SET(&ev, shutdownEventIdent, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    if (kevent(kq, &ev, 1, nullptr, 0, nullptr) == -1) {
        std::cerr << "[Server::start] Failed to register shutdown event\n";
        close(serverSocket);
        serverSocket = -1;
        close(kq);
        kq = -1;
        return;
    }
    running = true;
    eventThread = std::thread(&Server::eventLoop, this);
}

void Server::stop() {
    running = false;
    struct kevent ev {};
    EV_SET(&ev, shutdownEventIdent, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
    kevent(kq, &ev, 1, nullptr, 0, nullptr);
    if (eventThread.joinable()) {
        eventThread.join();
    }
    {
        for (const auto client : clientSockets) {
            close(client);
        }

        clientSockets.clear();
    }

    if (serverSocket != -1) {
        close(serverSocket);
        serverSocket = -1;
    }
    if (kq != -1) {
        close(kq);
        kq = -1;
    }
}
