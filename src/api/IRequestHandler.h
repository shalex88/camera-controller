#pragma once

#include "common/types/Result.h"
#include "common/types/ICameraOperations.h"

namespace camera_service::api {
    class IRequestHandler : public ICameraOperations {
    public:
        ~IRequestHandler() override = default;

        virtual Result<void> start() = 0;
        virtual Result<void> stop() = 0;
        virtual bool isRunning() const = 0;
    };
}
