//
// Created by Mosa Yaqoobi on 2026-06-24.
//

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace chat {
    class LevelBasedFormatter final : public spdlog::formatter {
        static constexpr const char* kPlainPattern  = "%v";
        static constexpr const char* kSimplePattern  = "[%^%l%$] %v";
        static constexpr const char* kVerbosePattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v";

        std::unique_ptr<spdlog::pattern_formatter> simple_;
        std::unique_ptr<spdlog::pattern_formatter> verbose_;
        std::unique_ptr<spdlog::pattern_formatter> plain_;

        public:
        LevelBasedFormatter()
            : simple_(std::make_unique<spdlog::pattern_formatter>(kSimplePattern)),
            verbose_(std::make_unique<spdlog::pattern_formatter>(kVerbosePattern)),
            plain_(std::make_unique<spdlog::pattern_formatter>(kPlainPattern)) {}


            void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override {
                if (msg.level == spdlog::level::debug || msg.level == spdlog::level::trace) {
                    verbose_->format(msg, dest);
                } else if (msg.level == spdlog::level::info) {
                    plain_->format(msg, dest);
                } else {
                    simple_->format(msg, dest);
                }
            }

            [[nodiscard]] std::unique_ptr<spdlog::formatter> clone() const override {
                return std::make_unique<LevelBasedFormatter>();
            }
    };

    inline void applyFormatter(const std::shared_ptr<spdlog::logger>& log) {
        log->set_formatter(std::make_unique<LevelBasedFormatter>());
    }

    inline void initLogging() {
        spdlog::flush_on(spdlog::level::err);
        spdlog::cfg::load_env_levels();
        if (!std::getenv("SPDLOG_LEVEL")) {
            spdlog::set_level(spdlog::level::info);
        }
    }

    inline std::shared_ptr<spdlog::logger> serverLogger() {
        static auto log = spdlog::stdout_color_mt("server");
        static bool formatted = false;
        if (!formatted) {
            applyFormatter(log);
            formatted = true;
        }
        return log;
    }


    inline std::shared_ptr<spdlog::logger> clientLogger() {
        static auto log = spdlog::stdout_color_mt("client");
        static bool formatted = false;
        if (!formatted) {
            applyFormatter(log);
            formatted = true;
        }
        return log;
    }
}
