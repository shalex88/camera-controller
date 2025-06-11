#pragma once
#include "ICamera.h"

namespace camera_service::data {
    class NfovCamera final : public ICamera {
    public:
        NfovCamera();
        ~NfovCamera() override;

        void setZoom(double zoomLevel) override;
        double getZoom() const override;
        void setFocus(double focusValue) override;
        double getFocus() const override;
        bool connect() override;
        void disconnect() override;
        bool isConnected() const override;

    private:
        double m_zoomLevel;
        double m_focusValue;
        bool m_connected;
    };
}