//
// Created by Mosa Yaqoobi on 2026-06-07.
//

#pragma once

#include <string>
#include <utility>

class Message {
public:
    int senderSocket{};
    std::string text;


    enum class MessageType {
        Chat,
        Join,
        Leave,
    };
    MessageType type {};

    /*
     * @senderSocket the client that sent the @text of @type (Chat, Join, Leave)
     */
    Message(const int senderSocket, std::string text, const MessageType type) :
        senderSocket(senderSocket),
        text(std::move(text)),
        type(type) {};

};