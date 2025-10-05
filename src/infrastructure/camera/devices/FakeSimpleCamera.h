#pragma once

#include "../hal/ICameraHw.h"
#include "common/types/CameraCapabilities.h"

namespace camera_service::infrastructure {
    class FakeSimpleCamera final : public ICameraHw,
                             public capabilities::IZoomCapable,
                             public capabilities::IFocusCapable,
                             public capabilities::IInfoCapable {
    public:
        FakeSimpleCamera() = default;
        ~FakeSimpleCamera() override = default;

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

    private:
        const types::ZoomRange zoom_limits_{
            .min = 0x0,
            .max = 0xFF
        };

        const types::FocusRange focus_limits_{
            .min = 0x0,
            .max = 0xFF
        };

        mutable types::zoom zoom_ = zoom_limits_.min;
        mutable types::focus focus_ = focus_limits_.min;
        types::info info_ = "Fake Simple Camera";
    };
}
