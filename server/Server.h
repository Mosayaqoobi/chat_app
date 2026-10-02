//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#pragma once

#include "chat/Message.h"

#include <unordered_set>
#include <thread>
#include <atomic>
#include <string>


class Server {
public:
    explicit Server(const uint16_t port) :
        port_(port) {}

    ~Server() { stop(); }

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /*
     * Creates the server socket, binds, listens, sets up kqueue,
     * and starts the event loop on a background thread.
     */
    void start();

    /*
     * Signals shutdown, wakes the event loop, joins the event thread,
     * and closes all client sockets, the server socket, and the kqueue.
     */
    void stop(const std::string& farewell = "");

    /*
     * Blocks on kqueue until a socket is ready or shutdown is requested.
     * Accepts new clients, reads incoming messages, and removes disconnected clients.
     */
    void eventLoop();

    /*
     * Accepts one pending connection when the listening socket is readable.
     * Registers the new client with kqueue and adds it to clientSockets.
     */
    void acceptNewClient();

    /*
     * Adds a client socket to clientSockets if it is not already present
     * and the client limit has not been reached.
     * Returns false if the client could not be added.
     */
    bool addClient(int clientSocket);

    /*
     * Removes a client socket from clientSockets and closes it.
     * Closing the socket also removes it from kqueue.
     */
    void removeClient(int clientSocket);

    void dropClient(const int clientSocket);

    /*
     * Sends a message to every connected client except the sender.
     * Removes any clients whose send() fails.
     */
    void broadcastMessage(const Message& message);

    /*
     * Reads one message from a client socket and broadcasts it to the others.
     * Removes the client if recv() indicates a disconnect or error.
     */
    void handleClient(int clientSocket);

    /*
     * Returns whether the server is currently running.
     */
    [[nodiscard]] bool isRunning() const { return running_; }

    /*
     * Returns the serverSocket
     */
    [[nodiscard]] int getServerSocket() const { return serverSocket_; }
private:
    static constexpr std::size_t kMaxClients = 10000;
    uint16_t port_;

    std::size_t maxClients_ {kMaxClients};

    int serverSocket_ {-1};  //when making a socket instance
    std::unordered_set<int> clientSockets_{};

    std::atomic<bool> running_ {false};
    std::thread eventThread_{};

    int kq_ {-1};
};
