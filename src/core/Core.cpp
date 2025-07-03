#include "Core.h"
#include <iostream>
#include "common/Logger/Logger.h"
#include "data/ICamera.h"

namespace camera_service::core {
    Core::Core(std::unique_ptr<data::ICamera> camera)
        : camera_(std::move(camera)), is_initialized_(false) {
        if (!camera_) {
            throw std::invalid_argument("Cannot initialize Core with null camera");
        }
    }

    Core::~Core() {
        if (isInitialized()) {
            shutdown();
        }
    }

    Result<void> Core::initialize() {
        LOG_INFO("Initializing Core...");

        if (!camera_->isConnected()) {
            if (const auto connect_result = camera_->connect(); connect_result.isError()) {
                return Result<void>::error(connect_result.error());
            }
        }

        is_initialized_ = true;
        LOG_INFO("Core initialized successfully.");
        return Result<void>::success();
    }

    Result<void> Core::shutdown() {
        if (!isInitialized()) {
            return Result<void>::success();
        }

        is_initialized_ = false;

        LOG_INFO("Shutting down Core...");

        if (camera_ && camera_->isConnected()) {
            if (const auto disconnect_result = camera_->disconnect(); disconnect_result.isError()) {
                return Result<void>::error(disconnect_result.error());
            }
        }

        LOG_INFO("Core shut down successfully.");
        return Result<void>::success();
    }

    bool Core::isInitialized() const {
        return is_initialized_;
    }

    Result<void> Core::setZoom(const types::zoom zoom_level) {
        if (!isInitialized()) {
            return Result<void>::error("Core not initialized");
        }

        return camera_->setZoom(zoom_level);
    }

    Result<types::zoom> Core::getZoom() const {
        if (!isInitialized()) {
            return Result<types::zoom>::error("Core not initialized");
        }

        return camera_->getZoom();
    }

    Result<void> Core::setFocus(const types::focus focus_value) {
        if (!isInitialized()) {
            return Result<void>::error("Core not initialized");
        }

        return camera_->setFocus(focus_value);
    }

    Result<types::focus> Core::getFocus() const {
        if (!isInitialized()) {
            return Result<types::focus>::error("Core not initialized");
        }

        return camera_->getFocus();
    }
}
