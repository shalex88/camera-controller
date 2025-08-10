#include "RequestHandler.h"

#include "core/Core.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    RequestHandler::RequestHandler(std::unique_ptr<core::ICore> core)
        : core_(std::move(core)), running_(false) {
        if (!core_) {
            throw std::invalid_argument("[API] Core cannot be null");
        }
    }

    RequestHandler::~RequestHandler() {
        if (running_) {
            if (stop().isError()) {
                LOG_ERROR("[API] RequestHandler failed to stop gracefully");
            }
        }
    }

    Result<void> RequestHandler::start() {
        LOG_INFO("[API] Starting Request Handler...");

        if (const auto init_result = core_->initialize(); init_result.isError()) {
            return Result<void>::error("[API] Core initialization failed: " + init_result.error());
        }

        running_ = true;
        return Result<void>::success();
    }

    Result<void> RequestHandler::stop() {
        if (!isRunning()) {
            return Result<void>::success();
        }

        LOG_INFO("[API] Stopping Request Handler...");
        running_ = false;

        if (core_) {
            if (const auto shutdown_result = core_->shutdown(); shutdown_result.isError()) {
                LOG_ERROR("[API] Error stopping core: {}", shutdown_result.error());
                return Result<void>::error("[API] Failed to shut down core: " + shutdown_result.error());
            }
        }
        return Result<void>::success();
    }

    bool RequestHandler::isRunning() const {
        return running_;
    }

    Result<void> RequestHandler::setZoom(const types::zoom zoom_level) {
        if (!isRunning()) {
            return Result<void>::error("[API] Request Handler is not running");
        }

        LOG_INFO("[API] Request: SetZoom to {}", zoom_level);

        auto operation = core_->setZoom(zoom_level);

        if (operation.isError()) {
            LOG_ERROR("[API] Response: {}", operation.error());
        } else {
            LOG_INFO("[API] Response: Success");
        }

        return operation;
    }

    Result<types::zoom> RequestHandler::getZoom() const {
        if (!isRunning()) {
            return Result<types::zoom>::error("[API] Request Handler is not running");
        }

        LOG_INFO("[API] Request: GetZoom");

        auto operation = core_->getZoom();

        if (operation.isError()) {
            LOG_ERROR("[API] Response: {}", operation.error());
        } else {
            LOG_INFO("[API] Response: {}", operation.value());
        }

        return operation;
    }

    Result<void> RequestHandler::setFocus(const types::focus focus_value) {
        if (!isRunning()) {
            return Result<void>::error("[API] Request Handler is not running");
        }

        LOG_INFO("[API] Request: SetFocus to {}", focus_value);

        auto operation = core_->setFocus(focus_value);

        if (operation.isError()) {
            LOG_ERROR("[API] Response: {}", operation.error());
        } else {
            LOG_INFO("[API] Response: Success");
        }

        return operation;
    }

    Result<types::focus> RequestHandler::getFocus() const {
        if (!isRunning()) {
            return Result<types::focus>::error("[API] Request Handler is not running");
        }

        LOG_INFO("[API] Request: GetZoom");

        auto operation = core_->getFocus();

        if (operation.isError()) {
            LOG_ERROR("[API] Response: {}", operation.error());
        } else {
            LOG_INFO("[API] Response: {}", operation.value());
        }

        return operation;
    }
}
