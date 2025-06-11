#include "Camera.h"
#include <iostream>

#include "common/Logger/Logger.h"

namespace nfov {
namespace data {

Camera::Camera()
    : m_zoomLevel(1.0)
    , m_focusValue(0.0)
    , m_connected(false) {
}

Camera::~Camera() {
    if (m_connected) {
        disconnect();
    }
}

void Camera::setZoom(double zoomLevel) {
    if (!m_connected) {
        throw CameraException("Cannot set zoom: Camera not connected");
    }

    if (zoomLevel <= 0) {
        throw CameraException("Invalid zoom level: Value must be greater than zero");
    }

    // Here would be the actual implementation to control hardware
    LOG_INFO("Setting camera zoom to: {}", zoomLevel);
    m_zoomLevel = zoomLevel;
}

double Camera::getZoom() const {
    if (!m_connected) {
        throw CameraException("Cannot get zoom: Camera not connected");
    }
    return m_zoomLevel;
}

void Camera::setFocus(double focusValue) {
    if (!m_connected) {
        throw CameraException("Cannot set focus: Camera not connected");
    }

    // Here would be the actual implementation to control hardware
    LOG_INFO("Setting camera focus to: {}", focusValue);
    m_focusValue = focusValue;
}

double Camera::getFocus() const {
    if (!m_connected) {
        throw CameraException("Cannot get focus: Camera not connected");
    }
    return m_focusValue;
}

bool Camera::connect() {
    // Here would be the actual implementation to connect to hardware
    LOG_INFO("Connecting to camera...");
    m_connected = true;
    return m_connected;
}

void Camera::disconnect() {
    if (!m_connected) {
        return;
    }

    // Here would be the actual implementation to disconnect from hardware
    LOG_INFO("Disconnecting from camera...");
    m_connected = false;
}

bool Camera::isConnected() const {
    return m_connected;
}

} // namespace data
} // namespace nfov
