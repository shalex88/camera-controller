#include "common/runtime/ShutdownCoordinator.h"

#include <mutex>
#include <utility>

namespace service::common::runtime {
    namespace {
        std::mutex g_shutdown_handler_mutex;
        ShutdownHandler g_shutdown_handler;
    } // namespace

    void registerShutdownHandler(ShutdownHandler handler) {
        std::lock_guard lock(g_shutdown_handler_mutex);
        g_shutdown_handler = std::move(handler);
    }

    void clearShutdownHandler() {
        std::lock_guard lock(g_shutdown_handler_mutex);
        g_shutdown_handler = nullptr;
    }

    void requestShutdown() {
        ShutdownHandler handler;

        {
            std::lock_guard lock(g_shutdown_handler_mutex);
            handler = g_shutdown_handler;
        }

        if (handler) {
            handler();
        }
    }
} // namespace service::common::runtime
