#include "NfovCamera.h"

#include <iostream>

#include "common/Logger/Logger.h"

namespace camera_service::data {
    NfovCamera::NfovCamera()
        : zoom_level_(1.0)
          , focus_value_(0.0)
          , connected_(false) {
    }

    NfovCamera::~NfovCamera() {
        if (connected_) {
            disconnect();
        }
    }

    void NfovCamera::setZoom(const double zoom_level) {
        if (!connected_) {
            throw CameraException("Cannot set zoom: NFOV Camera not connected");
        }

        if (zoom_level <= 0) {
            throw CameraException("Invalid zoom level: Value must be greater than zero");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera zoom to: {}", zoom_level);
        zoom_level_ = zoom_level;
    }

    double NfovCamera::getZoom() const {
        if (!connected_) {
            throw CameraException("Cannot get zoom: NFOV Camera not connected");
        }
        return zoom_level_;
    }

    void NfovCamera::setFocus(const double focus_value) {
        if (!connected_) {
            throw CameraException("Cannot set focus: NFOV Camera not connected");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera focus to: {}", focus_value);
        focus_value_ = focus_value;
    }

    double NfovCamera::getFocus() const {
        if (!connected_) {
            throw CameraException("Cannot get focus: Camera not connected");
        }
        return focus_value_;
    }

    bool NfovCamera::connect() {
        // Here would be the actual implementation to connect to hardware
        LOG_INFO("Connecting to NFOV camera...");
        connected_ = true;
        return connected_;
    }

    void NfovCamera::disconnect() {
        if (!connected_) {
            return;
        }

        // Here would be the actual implementation to disconnect from hardware
        LOG_INFO("Disconnecting from NFOV camera...");
        connected_ = false;
    }

    bool NfovCamera::isConnected() const {
        return connected_;
    }
}