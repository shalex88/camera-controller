#pragma once
#include <atomic>
#include <memory>
#include <thread>

#include "api/IController.h"

namespace camera_service::core {
    class ICore;
}

namespace camera_service::api {
    class GrpcController final : public IController {
    public:
        explicit GrpcController(std::unique_ptr<core::ICore> core);
        ~GrpcController() override;

        bool startAsync() override;
        bool stop() override;

        bool isRunning() const override;

        bool setZoom(double zoom_level) override;
        double getZoom() const override;

        bool setFocus(double focus_value) override;
        double getFocus() const override;

    private:
        std::thread server_thread_;
        std::shared_ptr<core::ICore> core_;
        std::atomic<bool> running_;
        void runLoop() override;
    };
}
