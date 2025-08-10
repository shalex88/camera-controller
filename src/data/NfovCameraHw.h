#pragma once
#include "ICameraHw.h"

namespace camera_service::data {
    class NfovCameraHw final : public ICameraHw {
    public:
        NfovCameraHw() = default;
        ~NfovCameraHw() override;

        Result<void> setZoom(types::zoom value) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus value) override;
        Result<types::focus> getFocus() const override;
        Result<void> connect() override;
        Result<void> disconnect() override;
        types::CameraLimits getLimits() const override;

    private:
        types::CameraLimits limits_ {
            .min_zoom = 0.0,
            .max_zoom = 10.0,
            .min_focus = 0.0,
            .max_focus = 10.0,
        };
        types::zoom current_zoom_ {limits_.min_zoom};
        types::focus current_focus_ {limits_.min_focus};
        mutable std::mutex mutex_;
    };
}
