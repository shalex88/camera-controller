#include "WfovCamera.h"

#include <iostream>

#include "common/Logger/Logger.h"

namespace camera_service::data {
    WfovCamera::WfovCamera()
        : zoom_level_(1.0)
          , focus_value_(0.0)
          , connected_(false) {
    }

    WfovCamera::~WfovCamera() {
        if (connected_) {
            disconnect();
        }
    }

    void WfovCamera::setZoom(const types::zoom zoom_level) {
        if (!connected_) {
            throw CameraException("Cannot set zoom: WFOV Camera not connected");
        }

        if (zoom_level <= 0) {
            throw CameraException("Invalid zoom level: Value must be greater than zero");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera zoom to: {}", zoom_level);
        zoom_level_ = zoom_level;
    }

    types::zoom WfovCamera::getZoom() const {
        if (!connected_) {
            throw CameraException("Cannot get zoom: WFOV Camera not connected");
        }
        return zoom_level_;
    }

    void WfovCamera::setFocus(const types::focus focus_value) {
        if (!connected_) {
            throw CameraException("Cannot set focus: WFOV Camera not connected");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera focus to: {}", focus_value);
        focus_value_ = focus_value;
    }

    types::focus WfovCamera::getFocus() const {
        if (!connected_) {
            throw CameraException("Cannot get focus: WFOV Camera not connected");
        }
        return focus_value_;
    }

    bool WfovCamera::connect() {
        // Here would be the actual implementation to connect to hardware
        LOG_INFO("Connecting to Wfov camera...");
        connected_ = true;
        return connected_;
    }

    void WfovCamera::disconnect() {
        if (!connected_) {
            return;
        }

        // Here would be the actual implementation to disconnect from hardware
        LOG_INFO("Disconnecting from WFOV camera...");
        connected_ = false;
    }

    bool WfovCamera::isConnected() const {
        return connected_;
    }
}