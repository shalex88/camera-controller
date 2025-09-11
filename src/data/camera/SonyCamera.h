#pragma once

#include "../ICameraHw.h"

#include "data/hw_interface/uart/IUartInterface.h"
#include <memory>

namespace camera_service::data {
    class SonyCamera final : public ICameraHw {
    public:
        explicit SonyCamera(std::unique_ptr<uart::IUartInterface> uart_interface) :
            uart_(std::move(uart_interface)) {
        };
        ~SonyCamera() override;

        Result<void> setZoom(types::zoom zoom) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus) override;
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

        std::unique_ptr<uart::IUartInterface> uart_;

        static double convertToInt(const std::vector<char>& data);
        static std::vector<char> convertToVector(const double& value);
    };
}
