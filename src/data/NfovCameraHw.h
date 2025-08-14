#pragma once

#include "ICameraHw.h"

#include "registers_map_manager/RegistersMapManager.h"
#include <memory>

namespace camera_service::data {
    class NfovCameraHw final : public ICameraHw {
    public:
        explicit NfovCameraHw(std::unique_ptr<RegistersMapManager> fpga_manager) :
            fpga_(std::move(fpga_manager)) {
        };
        ~NfovCameraHw() override;

        Result<void> setZoom(types::zoom zoom) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus) override;
        Result<types::focus> getFocus() const override;
        Result<void> connect() override;
        Result<void> disconnect() override;
        types::CameraLimits getLimits() const override;

    private:
        types::CameraLimits limits_ {
            .min_zoom = 0.0,
            .max_zoom = 1.0,
            .min_focus = 0.0,
            .max_focus = 1.0,
        };
        types::zoom current_zoom_ {limits_.min_zoom};
        types::focus current_focus_ {limits_.min_focus};
        mutable std::mutex mutex_;
        std::unique_ptr<RegistersMapManager> fpga_;
    };
}
