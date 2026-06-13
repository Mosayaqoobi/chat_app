//
// Created by Mosa Yaqoobi on 2026-06-13.
//

#pragma once

#include <cstddef>
#include <string_view>

namespace chat {
    inline constexpr std::size_t kMaxMessageSize = 200;
    inline constexpr std::string_view kQuitCommand = "/quit";
}


