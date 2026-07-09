//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#pragma once

#include "chat/Message.h"

#include <atomic>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>
#include <utility>


class Server {
    static constexpr std::size_t kMaxClients = 100;
    std::string ip {};
    int port {};
    std::size_t maxClients {kMaxClients};

    int serverSocket {-1};  //when making a socket instance
    std::vector<int> clientSockets {};

    std::atomic<bool> running {false};
    std::thread eventThread;

    int kq {-1};

public:
    Server(std::string ip, const int port) :
    ip(std::move(ip)),
    port(port) {};

    /*
     * Creates the server socket, binds, listens, sets up kqueue,
     * and starts the event loop on a background thread.
     */
    void start();

    /*
     * Signals shutdown, wakes the event loop, joins the event thread,
     * and closes all client sockets, the server socket, and the kqueue.
     */
    void stop();

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
    [[nodiscard]] bool isRunning() const { return running; }

    /*
     * Returns the serverSocket
     */
    [[nodiscard]] int getServerSocket() const { return serverSocket; }
};
