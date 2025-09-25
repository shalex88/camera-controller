#pragma once

#include "../ICameraHw.h"
#include "common/types/CameraCapabilities.h"

namespace camera_service::data {
    class FakeAdvancedCamera final : public ICameraHw,
                             public capabilities::IZoomCapable,
                             public capabilities::IFocusCapable,
                             public capabilities::IAutoFocusCapable,
                             public capabilities::IInfoCapable {
    public:
        FakeAdvancedCamera() = default;
        ~FakeAdvancedCamera() override = default;

        // IZoomCapable implementation
        Result<void> setZoom(types::zoom zoom) const override;
        Result<types::zoom> getZoom() const override;
        types::ZoomRange getZoomLimits() const override;

        // IFocusCapable implementation
        Result<void> setFocus(types::focus focus) const override;
        Result<types::focus> getFocus() const override;
        types::FocusRange getFocusLimits() const override;

        // IInfoCapable implementation
        Result<types::info> getInfo() const override;

        // ICameraHw implementation
        Result<void> connect() override;
        Result<void> disconnect() override;

        // IAutoFocusCapable implementation
        Result<void> enableAutoFocus(bool on) const override;
        Result<bool> isAutoFocusEnabled() const override;

    private:
        types::ZoomRange zoom_limits_{
            .min = 0x0,
            .max = 0xFF
        };

        types::FocusRange focus_limits_{
            .min = 0x0,
            .max = 0xFF
        };

        mutable types::zoom zoom_ = zoom_limits_.min;
        mutable types::focus focus_ = focus_limits_.min;
        mutable bool auto_focus_enabled_ = false;
        types::info info_ = "Fake Advanced Camera";
    };
}
