#pragma once

#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>

#include "LoggerInterface.h"

class SpdLogAdapter final : public LoggerInterface {
public:
    SpdLogAdapter() {
        auto stdout_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(std::cout, true);
        logger_ = std::make_shared<spdlog::logger>(APP_NAME, stdout_sink);
        spdlog::set_default_logger(logger_);
        logger_->set_level(spdlog::level::info);
    }

    ~SpdLogAdapter() override {
        spdlog::drop_all();
    }

    void setLogLevel(const LogLevel level) override {
        logger_->set_level(toSpdLogLevel(level));
    }

protected:
    void logImpl(const LogLevel level, const std::string &msg) override {
        logger_->log(toSpdLogLevel(level), msg);

        if (level == LogLevel::Critical) {
            throw std::runtime_error(msg);
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;

    static spdlog::level::level_enum toSpdLogLevel(const LogLevel level) {
        switch (level) {
            case LogLevel::Trace:
                return spdlog::level::trace;
            case LogLevel::Debug:
                return spdlog::level::debug;
            case LogLevel::Info:
                return spdlog::level::info;
            case LogLevel::Warn:
                return spdlog::level::warn;
            case LogLevel::Error:
                return spdlog::level::err;
            case LogLevel::Critical:
                return spdlog::level::critical;
            default:
                throw std::invalid_argument("Invalid log severity");
        }
    }
};