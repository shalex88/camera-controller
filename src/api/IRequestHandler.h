#pragma once

#include "common/types/Result.h"
#include "common/types/ICameraCapabilities.h"

namespace camera_service::api {
    class IRequestHandler : public ICameraCapabilities {
    public:
        ~IRequestHandler() override = default;

        virtual Result<void> start() = 0;
        virtual Result<void> stop() = 0;
        virtual bool isRunning() const = 0;
    };
}
