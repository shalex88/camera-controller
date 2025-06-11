#include "WfovCamera.h"

#include <iostream>

#include "common/Logger/Logger.h"

namespace camera_service::data {
    WfovCamera::WfovCamera()
        : m_zoomLevel(1.0)
          , m_focusValue(0.0)
          , m_connected(false) {
    }

    WfovCamera::~WfovCamera() {
        if (m_connected) {
            disconnect();
        }
    }

    void WfovCamera::setZoom(const double zoomLevel) {
        if (!m_connected) {
            throw CameraException("Cannot set zoom: WFOV Camera not connected");
        }

        if (zoomLevel <= 0) {
            throw CameraException("Invalid zoom level: Value must be greater than zero");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera zoom to: {}", zoomLevel);
        m_zoomLevel = zoomLevel;
    }

    double WfovCamera::getZoom() const {
        if (!m_connected) {
            throw CameraException("Cannot get zoom: WFOV Camera not connected");
        }
        return m_zoomLevel;
    }

    void WfovCamera::setFocus(const double focusValue) {
        if (!m_connected) {
            throw CameraException("Cannot set focus: WFOV Camera not connected");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera focus to: {}", focusValue);
        m_focusValue = focusValue;
    }

    double WfovCamera::getFocus() const {
        if (!m_connected) {
            throw CameraException("Cannot get focus: WFOV Camera not connected");
        }
        return m_focusValue;
    }

    bool WfovCamera::connect() {
        // Here would be the actual implementation to connect to hardware
        LOG_INFO("Connecting to Wfov camera...");
        m_connected = true;
        return m_connected;
    }

    void WfovCamera::disconnect() {
        if (!m_connected) {
            return;
        }

        // Here would be the actual implementation to disconnect from hardware
        LOG_INFO("Disconnecting from WFOV camera...");
        m_connected = false;
    }

    bool WfovCamera::isConnected() const {
        return m_connected;
    }
}