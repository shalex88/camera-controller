#pragma once
#include <atomic>
#include <memory>
#include <string>

#include "api/IControllerAdapter.h"

namespace camera_service::core {
    class ICore;
}

namespace camera_service::api {
    class ControllerException final : public std::runtime_error {
    public:
        explicit ControllerException(const std::string& message) : std::runtime_error(message) {
        }
    };

    class Controller final {
    public:
        explicit Controller(std::unique_ptr<core::ICore> core, std::unique_ptr<IControllerAdapter> controller_impl, const std::string& port);
        ~Controller();

        bool startAsync();
        bool stop();

        bool isRunning() const;

        bool setZoom(double zoom_level) const;
        double getZoom() const;

        bool setFocus(double focus_value) const;
        double getFocus() const;
        void runLoop() const;

    private:
        std::unique_ptr<IControllerAdapter> controller_impl_;
        std::unique_ptr<core::ICore> core_;
        std::atomic<bool> running_;
        std::string port_;
    };
}
