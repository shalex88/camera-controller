#pragma once
#include <string>
#include "common/types/Result.h"

namespace camera_service::api {
    class Controller; // forward declaration

    class ITransport {
    public:
        virtual ~ITransport() = default;

        virtual void setController(Controller* controller) = 0;  // Keep as void since it's a simple setter
        virtual Result<void> start(const std::string& port) = 0;
        virtual Result<void> stop() = 0;
        virtual Result<void> runLoop() = 0;
    };
}
