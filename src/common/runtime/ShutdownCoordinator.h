#pragma once

#include <functional>

namespace service::common::runtime {
    using ShutdownHandler = std::function<void()>;

    void registerShutdownHandler(ShutdownHandler handler);
    void clearShutdownHandler();
    void requestShutdown();
} // namespace service::common::runtime
