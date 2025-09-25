#pragma once

#include "common/types/Result.h"
#include "common/types/CameraTypes.h"

namespace camera_service::api {
    class IRequestHandler {
    public:
        virtual ~IRequestHandler() = default;

        virtual Result<void> start() = 0;
        virtual Result<void> stop() = 0;
        virtual bool isRunning() const = 0;

        virtual Result<void> setZoom(types::zoom zoom_level) const = 0;
        virtual Result<types::zoom> getZoom() const = 0;
        virtual Result<void> goToMinZoom() const = 0;
        virtual Result<void> goToMaxZoom() const = 0;

        virtual Result<void> setFocus(types::focus focus_value) const = 0;
        virtual Result<types::focus> getFocus() const = 0;
        virtual Result<void> enableAutoFocus(bool on) const = 0;
        virtual Result<bool> isAutoFocusEnabled() const = 0;

        virtual Result<types::info> getInfo() const = 0;
    };
}
