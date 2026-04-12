#pragma once

#include "common/types/CameraCapabilities.h"
#include "common/types/CameraTypes.h"
#include "infrastructure/camera/hal/ICameraHw.h"

namespace service::infrastructure {
    class ItlProtocol;

    class MwirCamera final : public ICameraHw,
                             public common::capabilities::IZoomCapable,
                             public common::capabilities::IFocusCapable,
                             public common::capabilities::IAutoFocusCapable,
                             public common::capabilities::IInfoCapable {
    public:
        explicit MwirCamera(std::unique_ptr<ItlProtocol> protocol);
        ~MwirCamera() override;

        // IZoomCapable implementation
        Result<void> setZoom(common::types::zoom zoom) const override;
        Result<common::types::zoom> getZoom() const override;
        common::types::ZoomRange getZoomLimits() const override;

        // IFocusCapable implementation
        Result<void> setFocus(common::types::focus focus) const override;
        Result<common::types::focus> getFocus() const override;
        common::types::FocusRange getFocusLimits() const override;

        // IInfoCapable implementation
        Result<common::types::info> getInfo() const override;

        // ICameraHw implementation
        Result<void> open() override;
        Result<void> close() override;

        // IAutoFocusCapable implementation
        Result<void> enableAutoFocus(bool enable) const override;
        Result<bool> isAutoFocusEnabled() const override;

    private:
        std::unique_ptr<ItlProtocol> protocol_;
        mutable common::types::zoom zoom_;
        mutable common::types::focus focus_;
        mutable bool auto_focus_enabled_{true};
    };
} // namespace service::infrastructure
