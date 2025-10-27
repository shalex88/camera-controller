#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "LoggerInterface.h"
#include "SpdLogAdapter.h"

namespace camera_service::common {
#define CAMERA_SERVICE_LOGGER_SCOPE_COUNT 4

    enum class LogScope : std::uint8_t {
        Global = 0,
        Api,
        Core,
        Infrastructure
    };

    class ScopedLogger {
    public:
        ScopedLogger() = default;
        ScopedLogger(std::shared_ptr<LoggerInterface> logger, std::string scope_name) {
            reset(std::move(logger), std::move(scope_name));
        }

        void reset(std::shared_ptr<LoggerInterface> logger, std::string scope_name) {
            logger_impl_ = std::move(logger);
            scope_name_ = std::move(scope_name);
        }

        void setLogLevel(const LoggerInterface::LogLevel level) const {
            if (logger_impl_) {
                logger_impl_->setLogLevel(level);
            }
        }

        void setLogLevel(const std::string& level) const {
            if (logger_impl_) {
                logger_impl_->setLogLevel(level);
            }
        }

        template <typename... Args>
        void trace(const std::string& format_str, Args&&... args) const {
            log(LoggerInterface::LogLevel::Trace, format_str, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void debug(const std::string& format_str, Args&&... args) const {
            log(LoggerInterface::LogLevel::Debug, format_str, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void info(const std::string& format_str, Args&&... args) const {
            log(LoggerInterface::LogLevel::Info, format_str, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void warn(const std::string& format_str, Args&&... args) const {
            log(LoggerInterface::LogLevel::Warn, format_str, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void error(const std::string& format_str, Args&&... args) const {
            log(LoggerInterface::LogLevel::Error, format_str, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void critical(const std::string& format_str, Args&&... args) const {
            log(LoggerInterface::LogLevel::Critical, format_str, std::forward<Args>(args)...);
        }

    private:
        std::shared_ptr<LoggerInterface> logger_impl_;
        std::string scope_name_;

        template <typename... Args>
        void log(LoggerInterface::LogLevel level, const std::string& format_str, Args&&... args) const {
            if (!logger_impl_) {
                return;
            }

            const std::string prefixed_format = "[" + scope_name_ + "] " + format_str;
            logger_impl_->log(level, prefixed_format, std::forward<Args>(args)...);
        }
    };

    class LoggerRegistry {
    public:
        LoggerRegistry(const LoggerRegistry&) = delete;
        LoggerRegistry& operator=(const LoggerRegistry&) = delete;

        static LoggerRegistry& instance() {
            static LoggerRegistry registry;
            return registry;
        }

        void initialize(const std::string& app_name, const std::string& log_level) {
            auto adapter = std::make_shared<SpdLogAdapter>(app_name);
            configure(std::move(adapter), log_level);
        }

        void setLoggerAdapter(std::shared_ptr<LoggerInterface> adapter, const std::string& log_level) {
            configure(std::move(adapter), log_level);
        }

        ScopedLogger& getLogger(const LogScope scope) {
            return scoped_loggers_[static_cast<std::size_t>(scope)];
        }

        const ScopedLogger& getLogger(const LogScope scope) const {
            return scoped_loggers_[static_cast<std::size_t>(scope)];
        }

        void setLogLevel(const std::string& level) const {
            if (logger_impl_) {
                logger_impl_->setLogLevel(level);
            }
        }

        void setLogLevel(const LoggerInterface::LogLevel level) const {
            if (logger_impl_) {
                logger_impl_->setLogLevel(level);
            }
        }

    private:
        LoggerRegistry() {
            auto adapter = std::make_shared<SpdLogAdapter>();
            configure(std::move(adapter), "info");
        }

        void configure(std::shared_ptr<LoggerInterface> adapter, const std::string& log_level) {
            adapter->setLogLevel(log_level);
            logger_impl_ = std::move(adapter);

            scoped_loggers_[static_cast<std::size_t>(LogScope::Global)].reset(logger_impl_, "GLOBAL");
            scoped_loggers_[static_cast<std::size_t>(LogScope::Api)].reset(logger_impl_, "API");
            scoped_loggers_[static_cast<std::size_t>(LogScope::Core)].reset(logger_impl_, "CORE");
            scoped_loggers_[static_cast<std::size_t>(LogScope::Infrastructure)].reset(logger_impl_, "INFRASTRUCTURE");
        }

        std::shared_ptr<LoggerInterface> logger_impl_;
        std::array<ScopedLogger, CAMERA_SERVICE_LOGGER_SCOPE_COUNT> scoped_loggers_ {};
    };

    namespace detail {
        constexpr bool contains(std::string_view text, std::string_view pattern) {
            return text.find(pattern) != std::string_view::npos;
        }

        constexpr LogScope scopeFromFile(const char* file) {
            const std::string_view path(file);

            if (contains(path, "/src/api/") || contains(path, "\\src\\api\\")) {
                return LogScope::Api;
            }

            if (contains(path, "/src/core/") || contains(path, "\\src\\core\\")) {
                return LogScope::Core;
            }

            if (contains(path, "/src/infrastructure/") || contains(path, "\\src\\infrastructure\\")) {
                return LogScope::Infrastructure;
            }

            return LogScope::Global;
        }

        inline ScopedLogger& loggerFor(const char* file) {
            return LoggerRegistry::instance().getLogger(scopeFromFile(file));
        }
    }
}

#define CONFIGURE_GLOBAL_LOGGER(name, level) do { \
    camera_service::common::LoggerRegistry::instance().initialize((name), (level)); \
} while(false)

#define SET_LOG_LEVEL(level) camera_service::common::LoggerRegistry::instance().setLogLevel((level))

#define LOG_TRACE(...) camera_service::common::detail::loggerFor(__FILE__).trace(__VA_ARGS__)
#define LOG_DEBUG(...) camera_service::common::detail::loggerFor(__FILE__).debug(__VA_ARGS__)
#define LOG_INFO(...) camera_service::common::detail::loggerFor(__FILE__).info(__VA_ARGS__)
#define LOG_WARN(...) camera_service::common::detail::loggerFor(__FILE__).warn(__VA_ARGS__)
#define LOG_ERROR(...) camera_service::common::detail::loggerFor(__FILE__).error(__VA_ARGS__)
#define LOG_CRITICAL(...) camera_service::common::detail::loggerFor(__FILE__).critical(__VA_ARGS__)
