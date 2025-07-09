#pragma once
#include "common/types/Result.h"
#include "common/types/ICameraCapabilities.h"

namespace camera_service::data {
    class ICameraHw : public api::ICameraCapabilities {
    public:
        ~ICameraHw() override = default;

        virtual Result<void> connect() = 0;
        virtual Result<void> disconnect() = 0;
        virtual types::CameraLimits getLimits() const = 0;
    };
}