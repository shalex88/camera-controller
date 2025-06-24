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

    Result<void> WfovCamera::setZoom(const types::zoom zoom_level) {
        if (!connected_) {
            return Result<void>::error("Cannot set zoom: WFOV Camera not connected");
        }

        if (zoom_level <= 0) {
            return Result<void>::error("Invalid zoom level: Value must be greater than zero");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera zoom to: {}", zoom_level);
        zoom_level_ = zoom_level;
        return Result<void>::success();
    }

    Result<types::zoom> WfovCamera::getZoom() const {
        if (!connected_) {
            return Result<types::zoom>::error("Cannot get zoom: WFOV Camera not connected");
        }
        return Result<types::zoom>::success(zoom_level_);
    }

    Result<void> WfovCamera::setFocus(const types::focus focus_value) {
        if (!connected_) {
            return Result<void>::error("Cannot set focus: WFOV Camera not connected");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera focus to: {}", focus_value);
        focus_value_ = focus_value;
        return Result<void>::success();
    }

    Result<types::focus> WfovCamera::getFocus() const {
        if (!connected_) {
            return Result<types::focus>::error("Cannot get focus: WFOV Camera not connected");
        }
        return Result<types::focus>::success(focus_value_);
    }

    Result<void> WfovCamera::connect() {
        if (connected_) {
            return Result<void>::error("WFOV Camera already connected");
        }

        // Here would be the actual implementation to connect to hardware
        LOG_INFO("Connecting to WFOV camera");
        connected_ = true;
        return Result<void>::success();
    }

    Result<void> WfovCamera::disconnect() {
        if (!connected_) {
            return Result<void>::error("WFOV Camera not connected");
        }

        // Here would be the actual implementation to disconnect from hardware
        LOG_INFO("Disconnecting from WFOV camera");
        connected_ = false;
        return Result<void>::success();
    }

    bool WfovCamera::isConnected() const {
        return connected_;
    }
}