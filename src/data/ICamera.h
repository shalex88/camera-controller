#pragma once
#include "common/types/Result.h"
#include "common/types/CameraTypes.h"

namespace camera_service::data {
    class ICamera {
    public:
        virtual ~ICamera() = default;

        virtual Result<void> setZoom(types::zoom zoom_level) = 0;
        virtual Result<types::zoom> getZoom() const = 0;
        virtual Result<void> setFocus(types::focus focus_value) = 0;
        virtual Result<types::focus> getFocus() const = 0;
        virtual Result<void> connect() = 0;
        virtual Result<void> disconnect() = 0;
        virtual bool isConnected() const = 0;  // Keep this as bool since it's a simple state check
    };
}