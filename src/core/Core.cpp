#include "Core.h"

#include "common/logger/Logger.h"
#include "infrastructure/camera/hal/ICamera.h"

namespace camera_service::core {
    Core::Core(std::unique_ptr<infrastructure::ICamera> camera,
               std::shared_ptr<common::LayerLogger> logger)
        : camera_(std::move(camera)), logger_(std::move(logger)), is_initialized_(false) {
        if (!camera_) {
            throw std::invalid_argument("Cannot initialize Core with null camera");
        }
        if (!logger_) {
            throw std::invalid_argument("Logger cannot be null");
        }
    }

    Core::~Core() {
        if (isInitialized()) {
            if (shutdown().isError()) {
                logger_->error("Failed to shut down Core properly");
            }
        }
    }

    Result<void> Core::initialize() {
        logger_->info("Initializing...");

        if (!camera_->isConnected()) {
            if (const auto connect_result = camera_->connect(); connect_result.isError()) {
                return Result<void>::error(logger_, "Init failed: " + connect_result.error());
            }
        }

        is_initialized_ = true;
        logger_->info("Initialized successfully");
        return Result<void>::success();
    }

    Result<void> Core::shutdown() {
        if (!isInitialized()) {
            return Result<void>::success();
        }

        is_initialized_ = false;

        logger_->info("Shutting down Core...");

        if (camera_ && camera_->isConnected()) {
            if (const auto disconnect_result = camera_->disconnect(); disconnect_result.isError()) {
                return Result<void>::error(logger_, disconnect_result.error());
            }
        }

        logger_->info("Core shut down successfully");
        return Result<void>::success();
    }

    bool Core::isInitialized() const {
        return is_initialized_;
    }

    Result<void> Core::setZoom(const types::zoom zoom_level) const {
        if (!isInitialized()) {
            return Result<void>::error("Core is not initialized");
        }

        return camera_->setZoom(zoom_level);
    }

    Result<types::zoom> Core::getZoom() const {
        if (!isInitialized()) {
            return Result<types::zoom>::error("Core is not initialized");
        }

        return camera_->getZoom();
    }

    Result<void> Core::goToMinZoom() const {
        return setZoom(types::MIN_NORMALIZED_ZOOM);
    }

    Result<void> Core::goToMaxZoom() const {
        return setZoom(types::MAX_NORMALIZED_ZOOM);
    }

    Result<void> Core::setFocus(const types::focus focus_value) const {
        if (!isInitialized()) {
            return Result<void>::error("Core is not initialized");
        }

        return camera_->setFocus(focus_value);
    }

    Result<types::focus> Core::getFocus() const {
        if (!isInitialized()) {
            return Result<types::focus>::error("Core is not initialized");
        }

        return camera_->getFocus();
    }

    Result<void> Core::enableAutoFocus(const bool on) const {
        if (!isInitialized()) {
            return Result<void>::error("Core is not initialized");
        }

        return camera_->enableAutoFocus(on);
    }

    Result<types::info> Core::getInfo() const {
        if (!isInitialized()) {
            return Result<types::info>::error("Core is not initialized");
        }

        return camera_->getInfo();
    }

    Result<void> Core::stabilize(const bool on) const {
        if (!isInitialized()) {
            return Result<void>::error("Core is not initialized");
        }

        return camera_->stabilize(on);
    }
}
