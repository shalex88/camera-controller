#pragma once
#include <memory>

#include "api/IController.h"

namespace camera_service::core {
    class ICore;
}

namespace camera_service::api {
    class TcpController final : public IController {
    public:
        explicit TcpController(std::unique_ptr<core::ICore> core);
        ~TcpController() override;

        bool start() override;
        void stop() override;

        bool setZoom(double zoomLevel) override;
        double getZoom() override;

        bool setFocus(double focusValue) override;
        double getFocus() override;

    private:
        std::shared_ptr<core::ICore> m_core;
        bool m_running;
    };
}
