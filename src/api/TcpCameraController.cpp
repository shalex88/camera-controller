#include "TcpCameraController.h"

#include <iostream>

#include "core/CameraCore.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    TcpCameraController::TcpCameraController(std::unique_ptr<core::ICore> core)
        : m_core(std::move(core)), m_running(false) {
        if (!m_core) {
            throw ControllerException("Cannot initialize CameraController with null Core");
        }
    }

    TcpCameraController::~TcpCameraController() {
        if (m_running) {
            stop();
        }
    }

    bool TcpCameraController::start() {
        LOG_INFO("Starting TCP API Controller...");

        try {
            if (!m_core->initialize()) {
                throw ControllerException("Failed to initialize core component");
            }

            m_running = true;
            LOG_INFO("TCP API Controller started successfully.");
            return true;
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error during controller start: ") + e.what());
        }
    }

    void TcpCameraController::stop() {
        if (!m_running) {
            return;
        }

        LOG_INFO("Stopping NFOV Camera Controller...");

        try {
            m_core->shutdown();
            LOG_INFO("Camera Controller stopped successfully.");
        } catch (const core::CoreException& e) {
            // FIXME: Is it a good exception handling?
            LOG_ERROR("Error during controller shutdown: {}", e.what());
        }
        m_running = false;
    }

    bool TcpCameraController::setZoom(double zoomLevel) {
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

    double TcpCameraController::getZoom() {
        if (!m_running) {
            throw ControllerException("Controller not running");
        }

        try {
            return m_core->getZoom();
        } catch (const core::CoreException& e) {
            throw ControllerException(std::string("Core error retrieving zoom: ") + e.what());
        }
    }

    bool TcpCameraController::setFocus(double focusValue) {
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

    double TcpCameraController::getFocus() {
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
