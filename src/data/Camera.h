#pragma once
#include "ICamera.h"

#include <memory>

#include "ICameraStrategy.h"

namespace camera_service::data {
    class Camera final : public ICamera {
    public:
        explicit Camera(std::unique_ptr<ICameraStrategy> camera_strategy);
        ~Camera() override;

        Result<void> setZoom(types::zoom zoom) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus) override;
        Result<types::focus> getFocus() const override;
        Result<void> connect() override;
        Result<void> disconnect() override;
        bool isConnected() const override;

    private:
        std::unique_ptr<ICameraStrategy> camera_impl_;
        bool connected_ {false};
        bool isValidZoom(types::zoom value) const;
        bool isValidFocus(types::focus value) const;
        types::CameraLimits limits_;
    };
}