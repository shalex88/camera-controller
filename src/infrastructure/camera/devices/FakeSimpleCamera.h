#pragma once

#include <mutex>

#include "common/types/CameraCapabilities.h"
#include "infrastructure/camera/hal/ICameraHw.h"

namespace service::infrastructure {
    class FakeSimpleCamera final : public ICameraHw,
                             public common::capabilities::IZoomCapable,
                             public common::capabilities::IFocusCapable,
                             public common::capabilities::IInfoCapable {
    public:
        FakeSimpleCamera() = default;
        ~FakeSimpleCamera() override = default;

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

    private:
        mutable std::mutex state_mutex_;
        mutable common::types::zoom zoom_{0};
        mutable common::types::focus focus_{0};
        const common::types::info info_{"Fake Simple Camera"};
    };
} // namespace service::infrastructure
