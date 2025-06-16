#pragma once

#include <memory>

#include "LoggerInterface.h"
#include "SpdLogAdapter.h"

class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogLevel(const LoggerInterface::LogLevel level) const {
        logger_adapter_->setLogLevel(level);
    }

    template<typename... Args>
    void log(LoggerInterface::LogLevel level, const std::string& format, Args... args) {
        logger_adapter_->log(level, format, std::forward<Args>(args)...);
    }

    void setLoggerAdapter(std::unique_ptr<LoggerInterface> adapter) {
        logger_adapter_ = std::move(adapter);
    }

private:
    Logger() = default;

    std::unique_ptr<LoggerInterface> logger_adapter_ = std::make_unique<SpdLogAdapter>();
};

#define SET_LOG_LEVEL(level) Logger::getInstance().setLogLevel(level)

#define LOG_TRACE(...) Logger::getInstance().log(LoggerInterface::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) Logger::getInstance().log(LoggerInterface::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().log(LoggerInterface::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().log(LoggerInterface::LogLevel::Warn, __VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().log(LoggerInterface::LogLevel::Error, __VA_ARGS__)
#define LOG_CRITICAL(...) Logger::getInstance().log(LoggerInterface::LogLevel::Critical, __VA_ARGS__)