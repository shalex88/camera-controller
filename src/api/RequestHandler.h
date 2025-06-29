#pragma once
#include <atomic>
#include <memory>

#include "core/ICore.h"
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"
#include "api/IRequestHandler.h"

namespace camera_service::api {
    class RequestHandler : public IRequestHandler {
    public:
        explicit RequestHandler(std::unique_ptr<core::ICore> core);
        ~RequestHandler() override;

        Result<void> start() override;
        Result<void> stop() override;
        bool isRunning() const override;

        Result<void> setZoom(types::zoom zoom_level) const override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus_value) const override;
        Result<types::focus> getFocus() const override;

    private:
        std::unique_ptr<core::ICore> core_;
        std::atomic<bool> running_;
    };
}
