#pragma once
#include <string>
#include "common/types/Result.h"

namespace camera_service::api {
    class ITransport {
    public:
        virtual ~ITransport() = default;

        virtual Result<void> start(const std::string& port) = 0;
        virtual Result<void> stop() = 0;
        virtual Result<void> runLoop() = 0;
    };
}
