#pragma once

#include "common/types/CameraCapabilities.h"
#include "common/types/Result.h"

namespace camera_service::infrastructure {
    class ICamera : public capabilities::IZoomCapable,
                       public capabilities::IFocusCapable,
                       public capabilities::IAutoFocusCapable,
                       public capabilities::IStabilizeCapable,
                       public capabilities::IInfoCapable {
    public:
        ~ICamera() override = default;

        virtual Result<void> connect() = 0;
        virtual Result<void> disconnect() = 0;
        virtual bool isConnected() const = 0;
    };
}