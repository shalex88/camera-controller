#include "GrpcCameraController.h"

#include <iostream>

#include "core/CameraCore.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    GrpcCameraController::GrpcCameraController(std::unique_ptr<core::ICore> core)
        : m_core(std::move(core)), m_running(false) {
        if (!m_core) {
            throw ControllerException("Cannot initialize CameraController with null Core");
        }
    }

    GrpcCameraController::~GrpcCameraController() {
        if (m_running) {
            stop();
        }
    }

    bool GrpcCameraController::start() {
        LOG_INFO("Starting GRPC API Controller...");

        try {
            if (!m_core->initialize()) {
                throw ControllerException("Failed to initialize core component");
            }

            m_running = true;
            LOG_INFO("GRPC API Controller started successfully.");
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during controller start: ") + e.what());
        } catch (const std::exception& e) {
            throw ControllerException(std::string("Error starting controller: ") + e.what());
        }
    }

    void GrpcCameraController::stop() {
        if (!m_running) {
            return;
        }

        LOG_INFO("Stopping NFOV Camera Controller...");

        try {
            m_core->shutdown();
            m_running = false;
            LOG_INFO("Camera Controller stopped successfully.");
        } catch (const std::exception& e) {
            LOG_ERROR("Error during controller shutdown: {}", e.what());
            // Still mark as stopped even if there was an error
            m_running = false;
        }
    }

    bool GrpcCameraController::setZoom(double zoomLevel) {
        if (!m_running) {
            throw ControllerException("Controller not running");
        }

        try {
            m_core->setZoom(zoomLevel);
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during zoom operation: ") + e.what());
        }
    }

    double GrpcCameraController::getZoom() {
        if (!m_running) {
            throw ControllerException("Controller not running");
        }

        try {
            return m_core->getZoom();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving zoom: ") + e.what());
        }
    }

    bool GrpcCameraController::setFocus(double focusValue) {
        if (!m_running) {
            throw ControllerException("Controller not running");
        }

        try {
            m_core->setFocus(focusValue);
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during focus operation: ") + e.what());
        }
    }

    double GrpcCameraController::getFocus() {
        if (!m_running) {
            throw ControllerException("Controller not running");
        }

        try {
            return m_core->getFocus();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving focus: ") + e.what());
        }
    }
}
