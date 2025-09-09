#pragma once

#include <memory>
#include <utility>

#include "LoggerInterface.h"
#include "SpdLogAdapter.h"

class LayerLogger {
public:
    LayerLogger(std::shared_ptr<LoggerInterface> logger, std::string layer_name, const std::string& log_level = "info")
        : logger_impl_(std::move(logger)), layer_name_(std::move(layer_name)) {
        logger_impl_->setLogLevel(log_level);
    }

    void setLogLevel(const auto level) const {
        logger_impl_->setLogLevel(level);
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
    std::shared_ptr<LoggerInterface> logger_impl_;
    std::string layer_name_;

    template<typename... Args>
    void log(LoggerInterface::LogLevel level, const std::string& format_str, Args&&... args) {
        const std::string prefixed_format = "[" + layer_name_ + "] " + format_str;
        logger_impl_->log(level, prefixed_format, std::forward<Args>(args)...);
    }
};

class GlobalLogger {
public:
    GlobalLogger(const GlobalLogger&) = delete;
    GlobalLogger& operator=(const GlobalLogger&) = delete;

    static GlobalLogger& getInstance() {
        static GlobalLogger instance;
        return instance;
    }

    void setLogLevel(const auto level) const {
        logger_impl_->setLogLevel(level);
    }

    template<typename... Args>
    void log(LoggerInterface::LogLevel level, const std::string& format, Args... args) {
        const std::string prefixed_format = "[APP] " + format;
        logger_impl_->log(level, prefixed_format, std::forward<Args>(args)...);
    }

    void setLoggerAdapter(std::unique_ptr<LoggerInterface> adapter) {
        logger_impl_ = std::move(adapter);
    }

private:
    GlobalLogger() = default;

    std::unique_ptr<LoggerInterface> logger_impl_ = std::make_unique<SpdLogAdapter>();
};

#define SET_LOGGER_NAME(name) GlobalLogger::getInstance().setLoggerAdapter(std::make_unique<SpdLogAdapter>(name))
#define SET_LOG_LEVEL(level) GlobalLogger::getInstance().setLogLevel(level)

#define LOG_TRACE(...) GlobalLogger::getInstance().log(LoggerInterface::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) GlobalLogger::getInstance().log(LoggerInterface::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) GlobalLogger::getInstance().log(LoggerInterface::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...) GlobalLogger::getInstance().log(LoggerInterface::LogLevel::Warn, __VA_ARGS__)
#define LOG_ERROR(...) GlobalLogger::getInstance().log(LoggerInterface::LogLevel::Error, __VA_ARGS__)
#define LOG_CRITICAL(...) GlobalLogger::getInstance().log(LoggerInterface::LogLevel::Critical, __VA_ARGS__)