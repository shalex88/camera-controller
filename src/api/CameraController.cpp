#include "CameraController.h"

#include <iostream>

#include "core/CameraCore.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    CameraController::CameraController(std::unique_ptr<core::ICore> core)
        : m_core(std::move(core)), m_running(false) {
        if (!m_core) {
            throw ControllerException("Cannot initialize CameraController with null core");
        }
    }

    CameraController::~CameraController() {
        if (m_running) {
            stop();
        }
    }

    bool CameraController::start() {
        LOG_INFO("Starting Camera Controller...");

        try {
            if (!m_core->initialize()) {
                throw ControllerException("Failed to initialize core component");
            }

            m_running = true;
            LOG_INFO("Camera Controller started successfully.");
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during controller start: ") + e.what());
        } catch (const std::exception& e) {
            throw ControllerException(std::string("Error starting controller: ") + e.what());
        }
    }

    void CameraController::stop() {
        if (!m_running) {
            return;
        }

        LOG_INFO("Stopping Camera Controller...");

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

    bool CameraController::setZoom(double zoomLevel) {
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

    double CameraController::getZoom() {
        if (!m_running) {
            throw ControllerException("Controller not running");
        }

        try {
            return m_core->getZoom();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving zoom: ") + e.what());
        }
    }

    bool CameraController::setFocus(double focusValue) {
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

    double CameraController::getFocus() {
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
