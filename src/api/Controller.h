#pragma once
#include <atomic>
#include <memory>
#include <string>

#include "api/ITransport.h"
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

namespace camera_service::core {
    class ICore;
}

namespace camera_service::api {
    class Controller final {
    public:
        explicit Controller(std::unique_ptr<core::ICore> core, std::unique_ptr<ITransport> transport, const std::string& port);
        ~Controller();

        Result<void> startAsync();
        Result<void> stop();
        // Keep as bool since it's a simple state check
        bool isRunning() const;
        Result<void> runLoop() const;

        Result<void> setZoom(types::zoom zoom_level) const;
        Result<types::zoom> getZoom() const;
        Result<void> setFocus(types::focus focus_value) const;
        Result<types::focus> getFocus() const;

    private:
        std::unique_ptr<ITransport> transport_;
        std::unique_ptr<core::ICore> core_;
        std::atomic<bool> running_;
        std::string port_;
    };
}
