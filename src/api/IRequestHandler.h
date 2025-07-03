#pragma once
#include <atomic>
#include <memory>

#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

namespace camera_service::api {
    class IRequestHandler {
    public:
        virtual ~IRequestHandler() = default;

        virtual Result<void> start() = 0;
        virtual Result<void> stop() = 0;
        virtual bool isRunning() const = 0;
    };
}
