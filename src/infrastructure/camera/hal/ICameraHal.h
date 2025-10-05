#pragma once

#include "common/types/Result.h"
#include "common/types/CameraCapabilities.h"

namespace camera_service::infrastructure {
    class ICameraHal : public capabilities::IZoomCapable,
                       public capabilities::IFocusCapable,
                       public capabilities::IAutoFocusCapable,
                       public capabilities::IStabilizeCapable,
                       public capabilities::IInfoCapable {
    public:
        ~ICameraHal() override = default;

        virtual Result<void> connect() = 0;
        virtual Result<void> disconnect() = 0;
        virtual bool isConnected() const = 0;
    };
}