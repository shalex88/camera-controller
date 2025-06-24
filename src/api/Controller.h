#pragma once
#include <atomic>
#include <memory>
#include <string>

#include "api/ITransport.h"
#include "common/types/CameraTypes.h"

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
        explicit Controller(std::unique_ptr<core::ICore> core, std::unique_ptr<ITransport> transport, const std::string& port);
        ~Controller();

        bool startAsync();
        bool stop();
        bool isRunning() const;
        void runLoop() const;

        void setZoom(types::zoom zoom_level) const;
        types::zoom getZoom() const;
        void setFocus(types::focus focus_value) const;
        types::focus getFocus() const;

    private:
        std::unique_ptr<ITransport> transport_;
        std::unique_ptr<core::ICore> core_;
        std::atomic<bool> running_;
        std::string port_;
    };
}
