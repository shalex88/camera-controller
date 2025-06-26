#pragma once
#include <atomic>
#include <memory>

#include "core/ICore.h"
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

namespace camera_service::api {
    class RequestHandler final {
    public:
        explicit RequestHandler(std::unique_ptr<core::ICore> core);
        ~RequestHandler();

        Result<void> startAsync();
        Result<void> stop();
        bool isRunning() const;

        Result<void> setZoom(types::zoom zoom_level) const;
        Result<types::zoom> getZoom() const;
        Result<void> setFocus(types::focus focus_value) const;
        Result<types::focus> getFocus() const;

    private:
        std::unique_ptr<core::ICore> core_;
        std::atomic<bool> running_;
    };
}
