#pragma once

#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

namespace camera_service::capabilities {
    class IZoomCapable {
    public:
        virtual ~IZoomCapable() = default;

        virtual Result<void> setZoom(types::zoom zoom_level) const = 0;
        virtual Result<types::zoom> getZoom() const = 0;

        virtual types::ZoomRange getZoomLimits() const = 0;
    };
}