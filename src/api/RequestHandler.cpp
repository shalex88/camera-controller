#include "RequestHandler.h"

#include "core/Core.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    RequestHandler::RequestHandler(std::unique_ptr<core::ICore> core, std::shared_ptr<LayerLogger> logger)
        : core_(std::move(core)), running_(false), logger_(std::move(logger)) {
        if (!core_) {
            throw std::invalid_argument("Core cannot be null");
        }
    }

    RequestHandler::~RequestHandler() {
        if (running_) {
            if (stop().isError()) {
                logger_->error("RequestHandler failed to stop gracefully");
            }
        }
    }

    Result<void> RequestHandler::start() {
        if (const auto init_result = core_->initialize(); init_result.isError()) {
            return Result<void>::error(init_result.error());
        }

        running_ = true;
        return Result<void>::success();
    }

    Result<void> RequestHandler::stop() {
        if (!isRunning()) {
            return Result<void>::success();
        }

        logger_->debug("Stopping Request Handler...");
        running_ = false;

        if (core_) {
            if (const auto shutdown_result = core_->shutdown(); shutdown_result.isError()) {
                logger_->error("Error stopping core: {}", shutdown_result.error());
                return Result<void>::error("Failed to shut down core: " + shutdown_result.error());
            }
        }
        return Result<void>::success();
    }

    bool RequestHandler::isRunning() const {
        return running_;
    }

    Result<void> RequestHandler::setZoom(const types::zoom zoom_level) const {
        if (!isRunning()) {
            return Result<void>::error("Request Handler is not running");
        }

        logger_->debug("Request: {} {}", __func__, zoom_level);

        auto operation = core_->setZoom(zoom_level);

        if (operation.isError()) {
            logger_->error("Response: {}", operation.error());
        } else {
            logger_->debug("Response: Success");
        }

        return operation;
    }

    Result<types::zoom> RequestHandler::getZoom() const {
        if (!isRunning()) {
            return Result<types::zoom>::error("Request Handler is not running");
        }

        logger_->debug("Request: {}", __func__);

        auto operation = core_->getZoom();

        if (operation.isError()) {
            logger_->error("Response: {}", operation.error());
        } else {
            logger_->debug("Response: {}", operation.value());
        }

        return operation;
    }

    Result<void> RequestHandler::goToMinZoom() const {
        if (!isRunning()) {
            return Result<void>::error("Request Handler is not running");
        }

        logger_->debug("Request: {}", __func__);

        auto operation = core_->goToMinZoom();

        if (operation.isError()) {
            logger_->error("Response: {}", operation.error());
        } else {
            logger_->debug("Response: Success");
        }

        return operation;
    }

    Result<void> RequestHandler::goToMaxZoom() const {
        if (!isRunning()) {
            return Result<void>::error("Request Handler is not running");
        }

        logger_->debug("Request: {}", __func__);

        auto operation = core_->goToMaxZoom();

        if (operation.isError()) {
            logger_->error("Response: {}", operation.error());
        } else {
            logger_->debug("Response: Success");
        }

        return operation;
    }

    Result<void> RequestHandler::setFocus(const types::focus focus_value) const {
        if (!isRunning()) {
            return Result<void>::error("Request Handler is not running");
        }

        logger_->debug("Request: {} {}", __func__, focus_value);

        auto operation = core_->setFocus(focus_value);

        if (operation.isError()) {
            logger_->error("Response: {}", operation.error());
        } else {
            logger_->debug("Response: Success");
        }

        return operation;
    }

    Result<types::focus> RequestHandler::getFocus() const {
        if (!isRunning()) {
            return Result<types::focus>::error("Request Handler is not running");
        }

        logger_->debug("Request: {}", __func__);

        auto operation = core_->getFocus();

        if (operation.isError()) {
            logger_->error("Response: {}", operation.error());
        } else {
            logger_->debug("Response: {}", operation.value());
        }

        return operation;
    }

    Result<void> RequestHandler::enableAutoFocus(bool on) const {
        if (!isRunning()) {
            return Result<void>::error("Request Handler is not running");
        }

        logger_->debug("Request: {} {}", __func__, on);

        auto operation = core_->enableAutoFocus(on);
        if (operation.isError()) {
            logger_->error("Response: {}", operation.error());
        } else {
            logger_->debug("Response: Success");
        }

        return operation;
    }

    Result<bool> RequestHandler::isAutoFocusEnabled() const {
        if (!isRunning()) {
            return Result<bool>::error("Request Handler is not running");
        }

        logger_->debug("Request: {}", __func__);

        auto operation = core_->isAutoFocusEnabled();
        if (operation.isError()) {
            logger_->error("Response: {}", operation.error());
        } else {
            logger_->debug("Response: {}", operation.value());
        }

        return operation;
    }

    Result<types::info> RequestHandler::getInfo() const {
        if (!isRunning()) {
            return Result<types::info>::error("Request Handler is not running");
        }

        logger_->debug("Request: {}", __func__);

        auto operation = core_->getInfo();

        if (operation.isError()) {
            logger_->error("Response: {}", operation.error());
        } else {
            logger_->debug("Response: {}", operation.value());
        }

        return operation;
    }
}
