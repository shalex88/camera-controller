#pragma once
#include "ICameraStrategy.h"

namespace camera_service::data {
    class NfovCamera final : public ICameraStrategy {
    public:
        NfovCamera() = default;
        ~NfovCamera() override;

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
            .max_zoom = 10.0,
            .min_focus = 0.0,
            .max_focus = 10.0,
        };
        types::zoom current_zoom_ {limits_.min_zoom};
        types::focus current_focus_ {limits_.min_focus};
    };
}
