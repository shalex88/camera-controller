#include "RequestHandler.h"

#include "core/Core.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    RequestHandler::RequestHandler(std::unique_ptr<core::ICore> core)
        : core_(std::move(core)), running_(false) {
        if (!core_) {
            throw std::invalid_argument("Core cannot be null");
        }
    }

    RequestHandler::~RequestHandler() {
        if (running_) {
            stop();
        }
    }

    Result<void> RequestHandler::start() {
        LOG_INFO("Starting Request Handler...");

        if (const auto init_result = core_->initialize(); init_result.isError()) {
            return Result<void>::error("Core initialization failed: " + init_result.error());
        }

        running_ = true;
        return Result<void>::success();
    }

    Result<void> RequestHandler::stop() {
        if (!isRunning()) {
            return Result<void>::success();
        }

        LOG_INFO("Stopping Request Handler...");
        running_ = false;

        if (core_) {
            if (const auto shutdown_result = core_->shutdown(); shutdown_result.isError()) {
                LOG_ERROR("Error stopping core: {}", shutdown_result.error());
                return Result<void>::error("Failed to shut down core: " + shutdown_result.error());
            }
        }
        return Result<void>::success();
    }

    bool RequestHandler::isRunning() const {
        return running_;
    }

    Result<void> RequestHandler::setZoom(const types::zoom zoom_level) {
        if (!isRunning()) {
            return Result<void>::error("Request Handler is not running");
        }

        return core_->setZoom(zoom_level);
    }

    Result<types::zoom> RequestHandler::getZoom() const {
        if (!isRunning()) {
            return Result<types::zoom>::error("Request Handler is not running");
        }

        return core_->getZoom();
    }

    Result<void> RequestHandler::setFocus(const types::focus focus_value) {
        if (!isRunning()) {
            return Result<void>::error("Request Handler is not running");
        }

        return core_->setFocus(focus_value);
    }

    Result<types::focus> RequestHandler::getFocus() const {
        if (!isRunning()) {
            return Result<types::focus>::error("Request Handler is not running");
        }

        return core_->getFocus();
    }
}
