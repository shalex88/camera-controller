#pragma once

#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

namespace camera_service::capabilities {
    class IFocusCapable {
    public:
        virtual ~IFocusCapable() = default;

        virtual Result<void> setFocus(types::focus focus_value) const = 0;
        virtual Result<types::focus> getFocus() const = 0;

        virtual types::FocusRange getFocusLimits() const = 0;
    };
}