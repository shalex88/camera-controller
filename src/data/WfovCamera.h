#pragma once
#include "ICamera.h"

namespace camera_service::data {
    class WfovCamera final : public ICamera {
    public:
        WfovCamera();
        ~WfovCamera() override;

        void setZoom(double zoom_level) override;
        double getZoom() const override;
        void setFocus(double focus_value) override;
        double getFocus() const override;
        bool connect() override;
        void disconnect() override;
        bool isConnected() const override;

    private:
        double zoom_level_;
        double focus_value_;
        bool connected_;
    };
}