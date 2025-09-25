#pragma once

#include "../ICameraHw.h"
#include "common/types/IZoomCapable.h"
#include "common/types/IInfoCapable.h"

namespace camera_service::data {
    class FakeSimpleCamera final : public ICameraHw,
                             public capabilities::IZoomCapable,
                             public capabilities::IInfoCapable {
    public:
        FakeSimpleCamera() = default;
        ~FakeSimpleCamera() override = default;

        // IZoomCapable implementation
        Result<void> setZoom(types::zoom zoom) const override;
        Result<types::zoom> getZoom() const override;
        types::ZoomRange getZoomLimits() const override;

        // IInfoCapable implementation
        Result<types::info> getInfo() const override;

        // ICameraHw implementation
        Result<void> connect() override;
        Result<void> disconnect() override;

    private:
        types::ZoomRange zoom_limits_{
            .min = 0x0,
            .max = 0xFF
        };

        mutable types::zoom zoom_ = zoom_limits_.min;
        types::info info_ = "Fake Simple Camera";
    };
}
