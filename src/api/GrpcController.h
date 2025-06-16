#pragma once
#include <memory>

#include "api/IController.h"

namespace camera_service::core {
    class ICore;
}

namespace camera_service::api {
    class GrpcController final : public IController {
    public:
        explicit GrpcController(std::unique_ptr<core::ICore> core);
        ~GrpcController() override;

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
