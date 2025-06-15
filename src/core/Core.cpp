#include "Core.h"

#include <iostream>

#include "common/Logger/Logger.h"
#include "data/ICamera.h"

namespace camera_service::core {
    Core::Core(std::unique_ptr<data::ICamera> camera)
        : m_camera(std::move(camera)), m_initialized(false) {
        if (!m_camera) {
            throw CoreException("Cannot initialize Core with null camera");
        }
    }

    Core::~Core() {
        if (m_initialized) {
            shutdown();
        }
    }

    bool Core::initialize() {
        LOG_INFO("Initializing Core...");

        if (!m_camera->isConnected()) {
            if (!m_camera->connect()) {
                throw CoreException("Failed to connect to camera");
            }
        }

        m_initialized = true;
        LOG_INFO("NFOV Core initialized successfully.");
        return true;
    }

    void Core::shutdown() {
        if (!m_initialized) {
            return;
        }

        LOG_INFO("Shutting down Core...");

        if (m_camera && m_camera->isConnected()) {
            m_camera->disconnect();
        }

        m_initialized = false;
        LOG_INFO("Core shut down successfully.");
    }

    void Core::setZoom(const double zoomLevel) {
        if (!m_initialized) {
            throw CoreException("Core not initialized");
        }

        try {
            m_camera->setZoom(zoomLevel);
        } catch (const data::CameraException& e) {
            throw CoreException(std::string("Camera error during zoom operation: ") + e.what());
        }
    }

    double Core::getZoom() const {
        if (!m_initialized) {
            throw CoreException("Core not initialized");
        }

        try {
            return m_camera->getZoom();
        } catch (const data::CameraException& e) {
            throw CoreException(std::string("Camera error retrieving zoom: ") + e.what());
        }
    }

    void Core::setFocus(const double focusValue) {
        if (!m_initialized) {
            throw CoreException("Core not initialized");
        }

        try {
            m_camera->setFocus(focusValue);
        } catch (const data::CameraException& e) {
            throw CoreException(std::string("Camera error during focus operation: ") + e.what());
        }
    }

    double Core::getFocus() const {
        if (!m_initialized) {
            throw CoreException("Core not initialized");
        }

        try {
            return m_camera->getFocus();
        } catch (const data::CameraException& e) {
            throw CoreException(std::string("Camera error retrieving focus: ") + e.what());
        }
    }
}
