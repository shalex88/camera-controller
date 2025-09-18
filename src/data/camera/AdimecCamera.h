#pragma once

#include "../ICameraHw.h"
#include "common/types/IZoomCapable.h"
#include "common/types/IFocusCapable.h"
#include "common/types/IInfoCapable.h"

#include "data/hw_interface/mmio/RegistersMapManager.h"
#include <memory>

namespace camera_service::data {
    class AdimecCamera final : public ICameraHw,
                               public capabilities::IZoomCapable,
                               public capabilities::IFocusCapable,
                               public capabilities::IInfoCapable {
    public:
        explicit AdimecCamera(std::unique_ptr<RegistersMapManager> fpga_manager);
        ~AdimecCamera() override = default;

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
        types::ZoomRange zoom_limits_{
            .min = 0,
            .max = 100
        };

        types::FocusRange focus_limits_{
            .min = 0,
            .max = 100
        };

        std::unique_ptr<RegistersMapManager> fpga_;
    };
}
