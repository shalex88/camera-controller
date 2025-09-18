#pragma once
#include "common/types/Result.h"
#include "common/types/CameraTypes.h"

namespace camera_service::data {
    class ICameraHw {
    public:
        virtual ~ICameraHw() = default;

        virtual Result<void> connect() = 0;
        virtual Result<void> disconnect() = 0;
    };
}