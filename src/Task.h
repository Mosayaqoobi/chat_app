//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#ifndef LEARN_CPP_TASK_H
#define LEARN_CPP_TASK_H

#include <utility>
#include <optional>
#include "Message.h"


class Task {
public:
    std::optional<Message> message{};
    int clientSocket {-1};
    enum class TaskType {
        ReadFromClient,
        BroadcastMessage,
        DisconnectClient,
    };
    TaskType type {};

    Task(Message message, const TaskType type) :
        message(std::move(message)),
        type(type) {};

    Task(const int clientSocket, const TaskType type) :
        clientSocket(clientSocket),
        type(type) {};

    Task() = default;

};

#endif //LEARN_CPP_TASK_H
