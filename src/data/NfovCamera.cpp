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

    Result<void> NfovCamera::setZoom(const types::zoom zoom_level) {
        if (!connected_) {
            return Result<void>::error("Cannot set zoom: NFOV Camera not connected");
        }

        if (zoom_level <= 0) {
            return Result<void>::error("Invalid zoom level: Value must be greater than zero");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera zoom to: {}", zoom_level);
        zoom_level_ = zoom_level;
        return Result<void>::success();
    }

    Result<types::zoom> NfovCamera::getZoom() const {
        if (!connected_) {
            return Result<types::zoom>::error("Cannot get zoom: NFOV Camera not connected");
        }
        return Result<types::zoom>::success(zoom_level_);
    }

    Result<void> NfovCamera::setFocus(const types::focus focus_value) {
        if (!connected_) {
            return Result<void>::error("Cannot set focus: NFOV Camera not connected");
        }

        // Here would be the actual implementation to control hardware
        LOG_INFO("Setting camera focus to: {}", focus_value);
        focus_value_ = focus_value;
        return Result<void>::success();
    }

    Result<types::focus> NfovCamera::getFocus() const {
        if (!connected_) {
            return Result<types::focus>::error("Cannot get focus: NFOV Camera not connected");
        }
        return Result<types::focus>::success(focus_value_);
    }

    Result<void> NfovCamera::connect() {
        if (connected_) {
            return Result<void>::error("NFOV Camera already connected");
        }

        // Here would be the actual implementation to connect to hardware
        LOG_INFO("Connecting to NFOV camera");
        connected_ = true;
        return Result<void>::success();
    }

    Result<void> NfovCamera::disconnect() {
        if (!connected_) {
            return Result<void>::error("NFOV Camera not connected");
        }

        // Here would be the actual implementation to disconnect from hardware
        LOG_INFO("Disconnecting from NFOV camera");
        connected_ = false;
        return Result<void>::success();
    }

    bool NfovCamera::isConnected() const {
        return connected_;
    }
}