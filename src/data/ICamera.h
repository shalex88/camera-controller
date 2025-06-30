#pragma once
#include "common/types/Result.h"
#include "common/types/CameraTypes.h"
#include "common/types/ICameraOperations.h"

namespace camera_service::data {
    class ICamera : public api::ICameraOperations {
    public:
        ~ICamera() override = default;

        virtual Result<void> connect() = 0;
        virtual Result<void> disconnect() = 0;
        virtual bool isConnected() const = 0;
    };
}