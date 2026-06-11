//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#ifndef LEARN_CPP_SERVER_H
#define LEARN_CPP_SERVER_H

#include <atomic>
#include <deque>
#include <condition_variable>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <utility>
#include "Message.h"
#include "Task.h"


class Server {
    std::string ip {};
    int port {};
    std::size_t maxClients {100};
    std::size_t maxThreads {10};

    int serverSocket {-1};  //when making a socket instance
    std::vector<int> clientSockets {};

    // Protects against adding, removing and handling clients
    std::mutex clientsMutex;
    // Protects against pushing and popping from queue, and chacking whether queue is empty
    std::mutex taskMutex;
    std::vector<std::thread> workers {};
    std::deque<Task> tasks {};
    std::condition_variable taskCondition;
    std::unordered_set<int> busyClients;

    std::atomic<bool> running {false};
    std::thread eventThread;

public:
    Server(std::string ip, const int port) :
    ip(std::move(ip)),
    port(port) {};

    /*
     * Creates the server socket, binds, listens, starts worker threads, and begins accepting clients
     * Basically the main function
     */
    void start();

    /*
     * Shuts down the server, closes the sockets and stops the workers
     */
    void stop();

    /*
     * Watches serverSocket + all client sockets with select().
     * Accepts new clients when serverSocket is readable,
     * enqueues ReadFromClient tasks when a client socket is readable.
     */
    void eventLoop();

    /*
     * Accepts one pending connection (only called when select says it's ready)
     */
    void acceptNewClient();

    /*
     * Stores a newly accept client socket
     */
    bool addClient(int clientSocket);

    /*
     * Removes a client socket from the list
     */
    void removeClient(int clientSocket);

    /*
     * Sends one clients message to every other connected client
     */
    void broadcastMessage(const Message& message);

    /*
     * Handles reading messages from one client, then forwarding it to other clients
     */
    void handleClient(int clientSocket);


    /*
     * Getter for the state of the server
     */
    [[nodiscard]] bool isRunning() const { return running; }

    /*
     * creates maxWorkers worker threads
     */
    void startWorkers();

    /*
     * Stops/joins the workers
     */
    void stopWorkers();

    /*
     * what each worker thread runs (its task)
     */
    void workerLoop();

    /*
     * adds work to the shared queue
     * tasks given by clients to:
     * Send messages between clients
     * remove clients
     * add clients
     * broadcast messages
     * receive messages
     */
    void enqueueTask(const Task& task);
};

#endif //LEARN_CPP_SERVER_H
