#pragma once

#include <memory>

#include "common/types/CameraCapabilities.h"
#include "infrastructure/camera/hal/ICameraHw.h"

namespace camera_service::infrastructure {
    class GenicamProtocol;
    class ItlProtocol;

    class AdimecCamera final : public ICameraHw,
                               public capabilities::IInfoCapable {
    public:
        explicit AdimecCamera(std::unique_ptr<GenicamProtocol> camera_protocol, std::unique_ptr<ItlProtocol> lens_protocol);
        ~AdimecCamera() override = default;

        Result<void> open() override;
        Result<void> close() override;
        Result<types::info> getInfo() const override;

    private:
        std::unique_ptr<GenicamProtocol> camera_protocol_;
        std::unique_ptr<ItlProtocol> lens_protocol_;
    };
}
