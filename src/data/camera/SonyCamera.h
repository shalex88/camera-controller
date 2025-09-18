#pragma once

#include "../ICameraHw.h"
#include "common/types/IZoomCapable.h"
#include "common/types/IFocusCapable.h"
#include "common/types/IInfoCapable.h"
#include "../../../../libVISCA2/visca/libvisca.h"

namespace camera_service::data {
    class SonyCamera final : public ICameraHw,
                             public capabilities::IZoomCapable,
                             public capabilities::IFocusCapable,
                             public capabilities::IInfoCapable {
    public:
        explicit SonyCamera(std::string device_path);
        ~SonyCamera() override = default;

        // IZoomCapable implementation
        Result<void> setZoom(types::zoom zoom) const override;
        Result<types::zoom> getZoom() const override;
        types::ZoomRange getZoomLimits() const override;

        // IFocusCapable implementation
        Result<void> setFocus(types::focus focus) const override;
        Result<types::focus> getFocus() const override;
        types::FocusRange getFocusLimits() const override;

        // IInfoCapable implementation
        Result<types::info> getInfo() const override;

        // ICameraHw implementation
        Result<void> connect() override;
        Result<void> disconnect() override;

    private:
        types::ZoomRange zoom_limits_{
            .min = 0x0000,
            .max = 0x4000
        };

        types::FocusRange focus_limits_{
            .min = 0x1000,
            .max = 0xF000
        };

        std::string device_path_ {};
        mutable int32_t camera_address_ {};
        mutable VISCAInterface_t interface_ {};
        mutable VISCACamera_t camera_ {};

        static Result<void> sendCommand(const std::function<uint32_t()>& command);
        static Result<uint16_t> sendInquiry(const std::function<uint32_t(uint16_t*)>& inquiry);
    };
}
