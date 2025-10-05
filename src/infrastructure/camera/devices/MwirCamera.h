#pragma once

#include "../hal/ICameraHw.h"
#include "infrastructure/camera/transport/ethernet/ItlProtocol.h"
#include "common/types/CameraCapabilities.h"
#include "common/types/CameraTypes.h"

namespace camera_service::infrastructure {
    class MwirCamera final : public ICameraHw,
                             public capabilities::IZoomCapable,
                             public capabilities::IFocusCapable,
                             public capabilities::IAutoFocusCapable,
                             public capabilities::IInfoCapable {
    public:
        explicit MwirCamera(std::unique_ptr<ItlProtocol> protocol);
        ~MwirCamera() override = default;

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
        Result<bool> isAutoFocusEnabled() const;

    private:
        const types::ZoomRange zoom_limits_{ //TODO: define real limits
            .min = 0x0,
            .max = 0xFF
        };

        const types::FocusRange focus_limits_{ //TODO: define real limits
            .min = 0x0,
            .max = 0xFF
        };

        mutable types::zoom zoom_ = zoom_limits_.min;
        mutable types::focus focus_ = focus_limits_.min;
        mutable bool auto_focus_enabled_ = true;
        std::unique_ptr<ItlProtocol> protocol_;
    };
}
