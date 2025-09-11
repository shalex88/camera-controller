#pragma once
#include <atomic>
#include <memory>

#include "core/ICore.h"
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"
#include "api/IRequestHandler.h"

namespace camera_service::api {
    class RequestHandler final : public IRequestHandler {
    public:
        explicit RequestHandler(std::unique_ptr<core::ICore> core, std::shared_ptr<LayerLogger> logger);
        ~RequestHandler() override;

        Result<void> start() override;
        Result<void> stop() override;
        bool isRunning() const override;

        Result<void> setZoom(types::zoom zoom_level) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus_value) override;
        Result<types::focus> getFocus() const override;
        Result<types::info> getInfo() const override;

    private:
        std::unique_ptr<core::ICore> core_;
        std::atomic<bool> running_;
        std::shared_ptr<LayerLogger> logger_;
    };
}
