#pragma once

#include <iostream>

#include "LoggerInterface.h"

class StdoutAdapter final : public LoggerInterface {
public:
    StdoutAdapter() = default;

    void setLogLevel(const LogLevel level) override {
        if (level <= LogLevel::Critical) {
            log_level_ = level;
        } else {
            throw std::invalid_argument("Invalid log severity");
        }
    }

protected:
    void logImpl(const LogLevel level, const std::string& msg) override {
        if (level >= log_level_) {
            std::cout << "[" << toSpdLogLevel(level) << "] " << msg << std::endl;
        }

        if (level == LogLevel::Error || level == LogLevel::Critical) {
            throw std::runtime_error(msg);
        }
    }

private:
    LogLevel log_level_ = LogLevel::Info;

    static std::string toSpdLogLevel(const LogLevel level) {
        switch (level) {
            case LogLevel::Trace:
                return "trace";
            case LogLevel::Debug:
                return "debug";
            case LogLevel::Info:
                return "info";
            case LogLevel::Warn:
                return "warning";
            case LogLevel::Error:
                return "error";
            case LogLevel::Critical:
                return "critical";
            default:
                throw std::invalid_argument("Invalid log severity");
        }
    }
};