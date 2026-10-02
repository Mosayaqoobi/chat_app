//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#include "Server.h"
#include "chat/Constants.h"

#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <cerrno>
#include <chrono>

namespace {
    constexpr int eventBatchSize = 1024;  //batch size for kevents in a kqueue
    constexpr int shutdownEventIdent = 1;
    constexpr int socketOptionEnabled = 1;
}

bool Server::addClient(const int clientSocket) {

    if (clientSockets_.contains(clientSocket)) {
        std::cout << "[Server::addClient] Client is already connected\n";
        return false;
    }
    if (clientSockets_.size() >= maxClients_) {
        std::cerr << "[Server::addClient] Client list is full\n";
        return false;
    }
    clientSockets_.insert(clientSocket);
    std::cerr << "[Server::addClient]" << " Client " << clientSocket << " has joined the server\n";
    return true;
}
void Server::dropClient(const int clientSocket) {
    if (clientSockets_.erase(clientSocket) == 0) {
        return;
    }
    close(clientSocket);
    std::cout << "[Server::dropClient] Client " << clientSocket << " removed\n";
}

void Server::removeClient(const int clientSocket) {
    if (!clientSockets_.contains(clientSocket)) {
        return;
    }
    dropClient(clientSocket);
    // for the clients
    const Message leaveMsg(clientSocket,
        "Client " + std::to_string(clientSocket) + " has disconnected",
        Message::MessageType::Leave);
    broadcastMessage(leaveMsg);
}

void Server::broadcastMessage(const Message& message) {
    std::vector<int> deadSockets {};
    for (const auto client : clientSockets_) {
        if (client == message.senderSocket) {
            continue;
        }
        if (send(client, message.text.data(), message.text.size(), 0) == -1) {
            std::cerr << "[Server::broadcastMessage] send to " << client << " failed: " << strerror(errno) << "\n";
            deadSockets.push_back(client);
        }
    }
    for (const auto client : deadSockets) {
        dropClient(client);
    }
}

void Server::handleClient(const int clientSocket) {
    char buffer[chat::kMaxMessageSize];
    if (std::string::size_type receivedBytes = recv(clientSocket, buffer, sizeof(buffer), 0); receivedBytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return;
        }
        std::cerr << "[Server::handleClient] recv from " << clientSocket << " failed: " << strerror(errno) << "\n";
        removeClient(clientSocket);

    } else if (receivedBytes == 0) {
        removeClient(clientSocket);

    } else {
        broadcastMessage({clientSocket, {buffer, receivedBytes}, Message::MessageType::Chat});
    }
}

void Server::acceptNewClient() {
    const int clientSocket = accept(serverSocket_, nullptr, nullptr);
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
    if (kevent(kq_, &ev, 1, nullptr, 0, nullptr) == -1) {
        std::cerr << "[Server::acceptNewClient] Failed to add event to kqueue\n";
        removeClient(clientSocket);
    }

}

void Server::eventLoop() {
    while (isRunning())
    {
        struct kevent events[eventBatchSize];

        const int n = kevent(kq_, nullptr, 0, events, eventBatchSize, nullptr);

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
            }
            const int fd = static_cast<int>(events[i].ident);

            if (fd == serverSocket_) {
                acceptNewClient();
            } else if (!clientSockets_.contains(fd)) {
                continue;
            } else if (events[i].flags & EV_EOF) {
                removeClient(fd);
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
    addrinfo hints {}, *res {}, *p{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV;

    if (int stat {}; (stat = getaddrinfo(nullptr, std::to_string(port_).c_str(), &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(stat));
        return;
    }

    for (p = res; p != nullptr; p = p->ai_next) {
        if ((serverSocket_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            std::cerr << "[Server::start] Failed to create server socket\n";
            continue;
        }

        if (setsockopt(serverSocket_, SOL_SOCKET, SO_NOSIGPIPE, &socketOptionEnabled, sizeof(socketOptionEnabled))) {
            close(serverSocket_);
            std::cerr << "[Server::start] setsockopt failed: " << strerror(errno) << "\n";
            serverSocket_ = -1;
            continue;
        }

        if (bind(serverSocket_, p->ai_addr, p->ai_addrlen) == -1) {
            close(serverSocket_);
            serverSocket_ = -1;
            std::cerr << "[Server::start] bind failed: " << strerror(errno) << "\n";
            continue;
        }
        break;
    }
    freeaddrinfo(res);

    if (serverSocket_ == -1) {
        std::cerr << "[Server::start] Could not bind to port " << port_ << "\n";
        return;
    }

    if (listen(serverSocket_, SOMAXCONN) == -1)
    {
        std::cerr << "[Server::start] Failed to listen on server socket\n";
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }
    kq_ = kqueue();
    if (kq_ == -1) {
        std::cerr << "[Server::start] Failed to create kqueue\n";
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    struct kevent ev{};
    EV_SET(&ev, serverSocket_, EVFILT_READ, EV_ADD, 0, 0, nullptr);

    if (kevent(kq_, &ev, 1, nullptr, 0, nullptr) == -1) {
        std::cerr << "[Server::start] Failed to add event to kqueue\n";
        close(serverSocket_);
        serverSocket_ = -1;
        close(kq_);
        kq_ = -1;
        return;
    }
    EV_SET(&ev, shutdownEventIdent, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    if (kevent(kq_, &ev, 1, nullptr, 0, nullptr) == -1) {
        std::cerr << "[Server::start] Failed to register shutdown event\n";
        close(serverSocket_);
        serverSocket_ = -1;
        close(kq_);
        kq_ = -1;
        return;
    }
    running_ = true;
    eventThread_ = std::thread(&Server::eventLoop, this);
}

void Server::stop(const std::string& farewell) {
    running_ = false;
    struct kevent ev {};
    EV_SET(&ev, shutdownEventIdent, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
    kevent(kq_, &ev, 1, nullptr, 0, nullptr);
    if (eventThread_.joinable()) {
        eventThread_.join();
    }
    if (!farewell.empty()) {
        broadcastMessage(Message(serverSocket_, farewell, Message::MessageType::Leave));
    }

    for (const auto client : clientSockets_) {
        close(client);
    }

    clientSockets_.clear();

    if (serverSocket_ != -1) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
    if (kq_ != -1) {
        close(kq_);
        kq_ = -1;
    }
}
