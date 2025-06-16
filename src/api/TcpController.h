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

        bool setZoom(double zoom_level) override;
        double getZoom() override;

        bool setFocus(double focus_value) override;
        double getFocus() override;

    private:
        std::shared_ptr<core::ICore> core_;
        bool running_;
    };
}
