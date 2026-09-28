//
// Created by Mosa Yaqoobi on 2026-09-27.
//

#pragma once

#include <charconv>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <optional>


namespace chat {
    struct Endpoint {
        std::string ip;
        uint16_t port {};
        static std::optional<Endpoint> parse(const std::string& ip, const std::string& portStr) {
            in_addr dummy {};
            if (inet_pton(AF_INET, std::string(ip).c_str(), &dummy) != 1) {
                return std::nullopt;
            }
            int port {};
            auto [ptr, ec] = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port);
            if (ec != std::errc{} || ptr != portStr.data() + portStr.size()) {
                return std::nullopt;
            }
            if (port < 1 || port > 65535) {
                return std::nullopt;
            }
            return Endpoint{.ip = std::string(ip), .port = static_cast<std::uint16_t>(port)};
        }
    };
}
