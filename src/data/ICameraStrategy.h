#pragma once
#include "common/types/Result.h"
#include "common/types/ICameraOperations.h"

namespace camera_service::data {
    class ICameraStrategy : public api::ICameraOperations {
    public:
        ~ICameraStrategy() override = default;

        virtual Result<void> connect() = 0;
        virtual Result<void> disconnect() = 0;
        virtual types::CameraLimits getLimits() const = 0;
    };
}