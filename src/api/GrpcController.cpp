#include "GrpcController.h"

#include <iostream>

#include "core/Core.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    GrpcController::GrpcController(std::unique_ptr<core::ICore> core)
        : core_(std::move(core)), running_(false) {
        if (!core_) {
            throw ControllerException("Cannot initialize CameraController with null Core");
        }
    }

    GrpcController::~GrpcController() {
        if (running_) {
            stop();
        }
    }

    bool GrpcController::start() {
        LOG_INFO("Starting GRPC API Controller...");

        try {
            if (!core_->initialize()) {
                throw ControllerException("Failed to initialize core component");
            }

            running_ = true;
            LOG_INFO("GRPC API Controller started successfully.");
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during controller start: ") + e.what());
        }
    }

    void GrpcController::stop() {
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

    bool GrpcController::setZoom(const double zoom_level) {
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

    double GrpcController::getZoom() {
        if (!running_) {
            throw ControllerException("Controller not running");
        }

        try {
            return core_->getZoom();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving zoom: ") + e.what());
        }
    }

    bool GrpcController::setFocus(const double focus_value) {
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

    double GrpcController::getFocus() {
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
