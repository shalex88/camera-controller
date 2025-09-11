#pragma once

#include "../ICameraHw.h"
#include "../../../../libVISCA2/visca/libvisca.h"

namespace camera_service::data {
    class SonyCameraVisca final : public ICameraHw {
    public:
        explicit SonyCameraVisca(std::string device_path);
        ~SonyCameraVisca() override;

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
            .min_zoom = 0x0000,
            .max_zoom = 0x4000,  // Standard VISCA zoom range
            .min_focus = 0x1000,
            .max_focus = 0xF000, // Standard VISCA focus range
        };

        std::string device_path_ {};
        mutable int32_t camera_address_ {};
        mutable VISCAInterface_t interface_ {};
        mutable VISCACamera_t camera_ {};
        bool is_connected_ = false;

        static Result<void> sendCommand(const std::function<uint32_t()>& command);
        static Result<uint16_t> sendInquiry(const std::function<uint32_t(uint16_t*)>& inquiry);
        static uint16_t convertZoomToVisca(types::zoom zoom);
        static types::zoom convertZoomFromVisca(uint16_t visca_zoom);
        static uint16_t convertFocusToVisca(types::focus focus);
        static types::focus convertFocusFromVisca(uint16_t visca_focus);
    };
}
