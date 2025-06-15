#include "TcpController.h"

#include <iostream>

#include "core/Core.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    TcpController::TcpController(std::unique_ptr<core::ICore> core)
        : core_(std::move(core)), running_(false) {
        if (!core_) {
            throw ControllerException("Cannot initialize CameraController with null Core");
        }
    }

    TcpController::~TcpController() {
        if (running_) {
            stop();
        }
    }

    bool TcpController::start() {
        LOG_INFO("Starting TCP API Controller...");

        try {
            if (!core_->initialize()) {
                throw ControllerException("Failed to initialize core component");
            }

            running_ = true;
            LOG_INFO("TCP API Controller started successfully.");
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during controller start: ") + e.what());
        }
    }

    void TcpController::stop() {
        if (!running_) {
            return;
        }

        LOG_INFO("Stopping NFOV Camera Controller...");

        try {
            core_->shutdown();
            LOG_INFO("Camera Controller stopped successfully.");
        } catch (const core::CoreException& e) {
            // FIXME: Is it a good exception handling?
            LOG_ERROR("Error during controller shutdown: {}", e.what());
        }
        running_ = false;
    }

    bool TcpController::setZoom(const double zoom_level) {
        if (!running_) {
            throw ControllerException("Controller not running");
        }

        try {
            core_->setZoom(zoom_level);
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during zoom operation: ") + e.what());
        }
    }

    double TcpController::getZoom() {
        if (!running_) {
            throw ControllerException("Controller not running");
        }

        try {
            return core_->getZoom();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving zoom: ") + e.what());
        }
    }

    bool TcpController::setFocus(const double focus_value) {
        if (!running_) {
            throw ControllerException("Controller not running");
        }

        try {
            core_->setFocus(focus_value);
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during focus operation: ") + e.what());
        }
    }

    double TcpController::getFocus() {
        if (!running_) {
            throw ControllerException("Controller not running");
        }

        try {
            return core_->getFocus();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving focus: ") + e.what());
        }
    }
}
