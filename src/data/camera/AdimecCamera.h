#pragma once

#include "../ICameraHw.h"

#include "data/hw_interface/mmio/RegistersMapManager.h"
#include <memory>

namespace camera_service::data {
    class AdimecCamera final : public ICameraHw {
    public:
        explicit AdimecCamera(std::unique_ptr<RegistersMapManager> fpga_manager) :
            fpga_(std::move(fpga_manager)) {
        };
        ~AdimecCamera() override;

        Result<void> setZoom(types::zoom zoom) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus) override;
        Result<types::focus> getFocus() const override;
        Result<void> connect() override;
        Result<void> disconnect() override;
        types::CameraLimits getLimits() const override;

    private:
        types::CameraLimits limits_ {
            .min_zoom = 0,
            .max_zoom = 100,
            .min_focus = 0,
            .max_focus = 100,
        };
        std::unique_ptr<RegistersMapManager> fpga_;
    };
}
