#pragma once
#include <memory>

#include "api/ICameraController.h"

namespace camera_service::core {
    class ICore;
}

namespace camera_service::api {
    class GrpcCameraController final : public ICameraController {
    public:
        explicit GrpcCameraController(std::unique_ptr<core::ICore> core);
        ~GrpcCameraController() override;

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
