#pragma once
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

namespace camera_service::api {
    class ICameraCapabilities {
    public:
        virtual ~ICameraCapabilities() = default;

        virtual Result<void> setZoom(types::zoom zoom_level) = 0; //FIXME: make const when camera is stateless
        virtual Result<types::zoom> getZoom() const = 0;
        virtual Result<void> setFocus(types::focus focus_value) = 0; //FIXME: make const when camera is stateless
        virtual Result<types::focus> getFocus() const = 0;
    };
}
