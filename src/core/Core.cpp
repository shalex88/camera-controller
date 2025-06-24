#include "Core.h"
#include <iostream>
#include "common/Logger/Logger.h"
#include "data/ICamera.h"

namespace camera_service::core {
    Core::Core(std::unique_ptr<data::ICamera> camera)
        : camera_(std::move(camera)), initialized_(false) {
        if (!camera_) {
            throw std::invalid_argument("Cannot initialize Core with null camera");
        }
    }

    Core::~Core() {
        if (initialized_) {
            shutdown();
        }
    }

    Result<void> Core::initialize() {
        LOG_INFO("Initializing Core...");

        if (!camera_->isConnected()) {
            auto connectResult = camera_->connect();
            if (connectResult.isError()) {
                return Result<void>::error(connectResult.error());
            }
        }

        initialized_ = true;
        LOG_INFO("Core initialized successfully.");
        return Result<void>::success();
    }

    Result<void> Core::shutdown() {
        if (!initialized_) {
            return Result<void>::success();
        }

        LOG_INFO("Shutting down Core...");

        if (camera_ && camera_->isConnected()) {
            auto disconnectResult = camera_->disconnect();
            if (disconnectResult.isError()) {
                return Result<void>::error(disconnectResult.error());
            }
        }

        initialized_ = false;
        LOG_INFO("Core shut down successfully.");
        return Result<void>::success();
    }

    Result<void> Core::setZoom(const types::zoom zoom_level) {
        if (!initialized_) {
            return Result<void>::error("Core not initialized");
        }

        return camera_->setZoom(zoom_level);
    }

    Result<types::zoom> Core::getZoom() const {
        if (!initialized_) {
            return Result<types::zoom>::error("Core not initialized");
        }

        return camera_->getZoom();
    }

    Result<void> Core::setFocus(const types::focus focus_value) {
        if (!initialized_) {
            return Result<void>::error("Core not initialized");
        }

        return camera_->setFocus(focus_value);
    }

    Result<types::focus> Core::getFocus() const {
        if (!initialized_) {
            return Result<types::focus>::error("Core not initialized");
        }

        return camera_->getFocus();
    }
}
