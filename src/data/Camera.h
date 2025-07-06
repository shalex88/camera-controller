#pragma once
#include "ICamera.h"

#include <memory>

namespace camera_service::data {
    class Camera final : public ICamera {
    public:
        explicit Camera(std::unique_ptr<ICamera> camera_strategy);
        ~Camera() override;

        Result<void> setZoom(types::zoom zoom_level) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus_value) override;
        Result<types::focus> getFocus() const override;
        Result<void> connect() override;
        Result<void> disconnect() override;
        bool isConnected() const override;

    private:
        std::unique_ptr<ICamera> camera_impl_;
        bool connected_ {false};
    };
}