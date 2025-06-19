#pragma once
#include <atomic>
#include <memory>
#include <string>

#include "api/IApiAdapter.h"
#include "api/IController.h"

namespace camera_service::core {
    class ICore;
}

namespace camera_service::api {
    class ApiController final : public IController {
    public:
        explicit ApiController(std::unique_ptr<core::ICore> core, std::unique_ptr<IApiAdapter> api_adapter, const std::string& port);
        ~ApiController() override;

        bool startAsync() override;
        bool stop() override;

        bool isRunning() const override;

        bool setZoom(double zoom_level) override;
        double getZoom() const override;

        bool setFocus(double focus_value) override;
        double getFocus() const override;
        void runLoop() override;

    private:
        std::unique_ptr<IApiAdapter> api_adapter_;
        std::unique_ptr<core::ICore> core_;
        std::atomic<bool> running_;
        std::string port_;
    };
}
