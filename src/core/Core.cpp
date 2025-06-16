#include "Core.h"

#include <iostream>

#include "common/Logger/Logger.h"
#include "data/ICamera.h"

namespace camera_service::core {
    Core::Core(std::unique_ptr<data::ICamera> camera)
        : camera_(std::move(camera)), initialized_(false) {
        if (!camera_) {
            throw CoreException("Cannot initialize Core with null camera");
        }
    }

    Core::~Core() {
        if (initialized_) {
            shutdown();
        }
    }

    bool Core::initialize() {
        LOG_INFO("Initializing Core...");

        if (!camera_->isConnected()) {
            if (!camera_->connect()) {
                throw CoreException("Failed to connect to camera");
            }
        }

        initialized_ = true;
        LOG_INFO("NFOV Core initialized successfully.");
        return true;
    }

    void Core::shutdown() {
        if (!initialized_) {
            return;
        }

        LOG_INFO("Shutting down Core...");

        if (camera_ && camera_->isConnected()) {
            camera_->disconnect();
        }

        initialized_ = false;
        LOG_INFO("Core shut down successfully.");
    }

    void Core::setZoom(const double zoom_level) {
        if (!initialized_) {
            throw CoreException("Core not initialized");
        }

        try {
            camera_->setZoom(zoom_level);
        } catch (const data::CameraException& e) {
            throw CoreException(std::string("Camera error during zoom operation: ") + e.what());
        }
    }

    double Core::getZoom() const {
        if (!initialized_) {
            throw CoreException("Core not initialized");
        }

        try {
            return camera_->getZoom();
        } catch (const data::CameraException& e) {
            throw CoreException(std::string("Camera error retrieving zoom: ") + e.what());
        }
    }

    void Core::setFocus(const double focus_value) {
        if (!initialized_) {
            throw CoreException("Core not initialized");
        }

        try {
            camera_->setFocus(focus_value);
        } catch (const data::CameraException& e) {
            throw CoreException(std::string("Camera error during focus operation: ") + e.what());
        }
    }

    double Core::getFocus() const {
        if (!initialized_) {
            throw CoreException("Core not initialized");
        }

        try {
            return camera_->getFocus();
        } catch (const data::CameraException& e) {
            throw CoreException(std::string("Camera error retrieving focus: ") + e.what());
        }
    }
}
