#pragma once

#include "common/types/CameraCapabilities.h"
#include "infrastructure/camera/hal/ICameraHw.h"

namespace camera_service::infrastructure {
    class ViscaProtocol;

    class SonyCamera final : public ICameraHw,
                             public capabilities::IZoomCapable,
                             public capabilities::IFocusCapable,
                             public capabilities::IAutoFocusCapable,
                             public capabilities::IStabilizeCapable,
                             public capabilities::IInfoCapable {
    public:
        SonyCamera(std::unique_ptr<ViscaProtocol> protocol);
        ~SonyCamera() override;

        // IZoomCapable implementation
        Result<void> setZoom(types::zoom zoom) const override;
        Result<types::zoom> getZoom() const override;
        types::ZoomRange getZoomLimits() const override;

        // IFocusCapable implementation
        Result<void> setFocus(types::focus focus) const override;
        Result<types::focus> getFocus() const override;
        types::FocusRange getFocusLimits() const override;

        // IAutoFocusCapable implementation
        Result<void> enableAutoFocus(bool on) const override;
        Result<bool> isAutoFocusEnabled() const;

        // IInfoCapable implementation
        Result<types::info> getInfo() const override;

        // IStabilizeCapable implementation
        Result<void> stabilize(bool on) const override;

        // ICameraHw implementation
        Result<void> connect() override;
        Result<void> disconnect() override;

    private:
        const types::ZoomRange zoom_limits_{
            .min = 0x0000,
            .max = 0x4000 // or 0x7AC0 for combined digital zoom
        };

        const types::FocusRange focus_limits_{
            .min = 0x1000,
            .max = 0xF000
        };

        std::unique_ptr<ViscaProtocol> protocol_ {};
    };
}
