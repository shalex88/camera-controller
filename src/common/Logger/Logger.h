#pragma once

#include <memory>

#include "LoggerInterface.h"
#include "SpdLogAdapter.h"

class LayerLogger {
public:
    LayerLogger(std::shared_ptr<LoggerInterface> logger, const std::string& layer_name)
        : logger_(std::move(logger)), layer_name_(layer_name) {}

    void setLogLevel(LoggerInterface::LogLevel level) const {
        logger_->setLogLevel(level);
    }

    template<typename... Args>
    void trace(const std::string& format_str, Args&&... args) {
        log(LoggerInterface::LogLevel::Trace, format_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const std::string& format_str, Args&&... args) {
        log(LoggerInterface::LogLevel::Debug, format_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const std::string& format_str, Args&&... args) {
        log(LoggerInterface::LogLevel::Info, format_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const std::string& format_str, Args&&... args) {
        log(LoggerInterface::LogLevel::Warn, format_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const std::string& format_str, Args&&... args) {
        log(LoggerInterface::LogLevel::Error, format_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(const std::string& format_str, Args&&... args) {
        log(LoggerInterface::LogLevel::Critical, format_str, std::forward<Args>(args)...);
    }

private:
    std::shared_ptr<LoggerInterface> logger_;
    std::string layer_name_;

    template<typename... Args>
    void log(LoggerInterface::LogLevel level, const std::string& format_str, Args&&... args) {
        const std::string prefixed_format = "[" + layer_name_ + "] " + format_str;
        logger_->log(level, prefixed_format, std::forward<Args>(args)...);
    }
};

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