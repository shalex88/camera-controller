#pragma once
#include "ICamera.h"

namespace camera_service::data {
    class NfovCamera final : public ICamera {
    public:
        NfovCamera();
        ~NfovCamera() override;

        void setZoom(types::zoom zoom_level) override;
        types::zoom getZoom() const override;
        void setFocus(types::focus focus_value) override;
        types::focus getFocus() const override;
        bool connect() override;
        void disconnect() override;
        bool isConnected() const override;

    private:
        //TODO: make stateless
        types::zoom zoom_level_;
        types::focus focus_value_;
        bool connected_;
    };
}