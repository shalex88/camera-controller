#pragma once

#include "../ICameraHw.h"

namespace camera_service::data {
    class FakeCamera final : public ICameraHw {
    public:
        FakeCamera() = default;
        ~FakeCamera() override = default;

        Result<void> setZoom(types::zoom zoom) const override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus) const override;
        Result<types::focus> getFocus() const override;
        Result<types::info> getInfo() const override;
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

        mutable types::zoom zoom_ = limits_.min_zoom;
        mutable types::focus focus_ = limits_.min_focus;
        types::info info_ = "Fake Camera";
    };
}
