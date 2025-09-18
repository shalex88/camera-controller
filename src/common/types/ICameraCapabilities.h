#pragma once

#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

namespace camera_service::api {
    class ICameraCapabilities {
    public:
        virtual ~ICameraCapabilities() = default;

        virtual Result<void> setZoom(types::zoom zoom_level) const = 0;
        virtual Result<types::zoom> getZoom() const = 0;
        virtual Result<void> setFocus(types::focus focus_value) const = 0;
        virtual Result<types::focus> getFocus() const = 0;
        virtual Result<types::info> getInfo() const = 0;
        virtual Result<void> setMinZoom() const = 0;
        virtual Result<void> setMaxZoom() const = 0;
    };
}
