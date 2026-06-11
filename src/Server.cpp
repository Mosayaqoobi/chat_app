//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#include "Server.h"

#include <condition_variable>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <algorithm>
#include <arpa/inet.h>
#include <netinet/in.h>

void Server::enqueueTask(const Task& task) {
    {
        std::lock_guard<std::mutex> lock(taskMutex);
        tasks.push_back(task);
    }
    taskCondition.notify_one();
}

bool Server::addClient(int clientSocket) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto client : clientSockets) {
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

void Server::removeClient(int clientSocket) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    const auto it = std::ranges::find(clientSockets, clientSocket);
    if (it == clientSockets.end()) {
        std::cerr << "[Server::removeClient] Client not found\n";
        return;
    }
    clientSockets.erase(it);
    busyClients.erase(clientSocket);
    close(clientSocket);
    std::cout << "[Server::removeClient] Client with Socket " << clientSocket << " has been removed\n";
}

void Server::broadcastMessage(const Message& message) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto client: clientSockets) {
        if (client != message.senderSocket) {
            if (send(client, message.text.data(), message.text.size(), 0) == -1) {
                std::cerr << "[Server::broadcastMessage] Failed to send message to client "
                << client << " from client " << message.senderSocket << "\n";
            }
        }
    }
}
// Depends on clientSockets, clientsMutex, and send().
// It loops through clients and sends to everyone except the sender.
void Server::handleClient(int clientSocket) {
    char buffer[200];
    ssize_t receivedBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (receivedBytes == -1) {
        std::cerr << "[Server::handleClient] Failed to receive message from client " <<
            clientSocket << "\n";
    } else if (receivedBytes == 0) {
        std::cerr << "[Server::handleClient] Client disconnected\n";
        removeClient(clientSocket);
    } else {
        std::string msg(buffer, receivedBytes);
        const Message message (clientSocket, msg, Message::MessageType::Chat);
        broadcastMessage(message);
    }
    std::lock_guard<std::mutex> lock(clientsMutex);
    busyClients.erase(clientSocket);
}

void Server::workerLoop() {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(taskMutex);
            taskCondition.wait(lock, [this] {
                return !tasks.empty() || !isRunning();
            });

            if (!isRunning() && tasks.empty()) {
                return;
            }

            task = tasks.front();
            tasks.pop_front();
        }

        if (task.type == Task::TaskType::ReadFromClient) {
            handleClient(task.clientSocket);
        } else if (task.type == Task::TaskType::BroadcastMessage) {
            if (!task.message) {
                std::cerr << "[Server::handleClient::broadcastMessage] Message is null\n";
            } else {
                broadcastMessage(task.message.value());
            }
        } else if (task.type == Task::TaskType::DisconnectClient) {
            removeClient(task.clientSocket);
        }
    }
}

void Server::startWorkers() {
    if (!workers.empty()) {
        return;
    } else if (!isRunning()) {
        return;
    }
    for (std::size_t i {0}; i < maxThreads; i++) {
        workers.emplace_back(&Server::workerLoop, this);
    }

}

void Server::stopWorkers() {
    if (workers.empty()) {
        return;
    }
    for (auto i {0u}; i < workers.size(); i++) {
        if (workers[i].joinable()) {
            workers[i].join();
        }
    }
    workers.clear();
}

void Server::acceptNewClient() {
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket == -1) {
        std::cerr << "[Server::acceptNewClient] Failed to accept client\n";
        return;
    }
    if (!addClient(clientSocket))
    {
        std::cerr << "[Server::acceptNewClient] Failed to add client\n";
        close(clientSocket);
    }
}

void Server::eventLoop() {
    while (isRunning())
    {
        std::vector<int> watched;
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto client : clientSockets) {

                if (busyClients.find(client) == busyClients.end()) {

                    watched.push_back(client);
                }
            }
        }
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);
        int maxFd = serverSocket;
        for (auto sock : watched) {
            FD_SET(sock, &readSet);
            maxFd = std::max(maxFd, sock);
        }

        timeval tv {};
        tv.tv_sec = 0;
        tv.tv_usec = 100'000;

        int ready = select(maxFd + 1, &readSet, nullptr, nullptr, &tv);

        if (ready == -1) {
            std::cerr << "[Server::eventLoop] Failed to select client socket\n";
            continue;
        }
        if (ready == 0) {
            continue;   //timeout
        }
        // New connection
        if (FD_ISSET(serverSocket, &readSet)) {
            acceptNewClient();
        }

        for (int sock : watched) {
            if (FD_ISSET(sock, &readSet)) {
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    busyClients.insert(sock);
                }
                enqueueTask(Task(sock, Task::TaskType::ReadFromClient));
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
    int opt {1};
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "[Server::start] Failed to parse ip address\n";
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
    running = true;
    startWorkers();
    eventThread = std::thread(&Server::eventLoop, this);
}

void Server::stop() {
    running = false;
    if (eventThread.joinable()) {
        eventThread.join();
    }
    taskCondition.notify_all();
    stopWorkers();
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto client : clientSockets) {
            close(client);
        }

        clientSockets.clear();
        busyClients.clear();
    }

    if (serverSocket != -1) {
        close(serverSocket);
        serverSocket = -1;
    }
}
