#pragma once
#include "ICamera.h"

namespace camera_service::data {
    class WfovCamera final : public ICamera {
    public:
        WfovCamera();
        ~WfovCamera() override;

        Result<void> setZoom(types::zoom zoom_level) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus_value) override;
        Result<types::focus> getFocus() const override;
        Result<void> connect() override;
        Result<void> disconnect() override;
        bool isConnected() const override;

    private:
        types::zoom zoom_level_;
        types::focus focus_value_;
        bool connected_;
    };
}