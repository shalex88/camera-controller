#pragma once

#include "../ICameraHw.h"
#include "../../../../libVISCA2/visca/libvisca.h"

namespace camera_service::data {
    class SonyCamera final : public ICameraHw {
    public:
        explicit SonyCamera(std::string device_path);
        ~SonyCamera() override = default;

        Result<void> setZoom(types::zoom zoom) const override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus) const override;
        Result<types::focus> getFocus() const override;
        Result<types::info> getInfo() const override;
        Result<void> setMinZoom() const override;
        Result<void> setMaxZoom() const override;
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

        static Result<void> sendCommand(const std::function<uint32_t()>& command);
        static Result<uint16_t> sendInquiry(const std::function<uint32_t(uint16_t*)>& inquiry);
    };
}
