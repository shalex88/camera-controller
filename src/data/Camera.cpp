#include "Camera.h"

#include <iostream>

#include "common/Logger/Logger.h"

namespace camera_service::data {
    Camera::Camera(std::unique_ptr<ICamera> camera_strategy) : camera_impl_(std::move(camera_strategy)) {
    }

    Camera::~Camera() {
        if (connected_) {
            disconnect();
        }
    }

    Result<void> Camera::setZoom(const types::zoom zoom_level) {
        if (!connected_) {
            return Result<void>::error("Cannot set zoom: NFOV Camera not connected");
        }

        if (zoom_level <= 0) {
            return Result<void>::error("Invalid zoom level: Value must be greater than zero");
        }

        LOG_INFO("Setting camera zoom to: {}", zoom_level);
        return camera_impl_->setZoom(zoom_level);
    }

    Result<types::zoom> Camera::getZoom() const {
        if (!connected_) {
            return Result<types::zoom>::error("Cannot get zoom: NFOV Camera not connected");
        }
        LOG_INFO("Getting camera zoom");
        return camera_impl_->getZoom();
    }

    Result<void> Camera::setFocus(const types::focus focus_value) {
        if (!connected_) {
            return Result<void>::error("Cannot set focus: NFOV Camera not connected");
        }

        LOG_INFO("Setting camera focus to: {}", focus_value);
        return camera_impl_->setFocus(focus_value);
    }

    Result<types::focus> Camera::getFocus() const {
        if (!connected_) {
            return Result<types::focus>::error("Cannot get focus: NFOV Camera not connected");
        }
        LOG_INFO("Getting camera focus");
        return camera_impl_->getFocus();
    }

    Result<void> Camera::connect() {
        if (connected_) {
            return Result<void>::error("NFOV Camera already connected");
        }

        LOG_INFO("Connecting to NFOV camera");
        if (camera_impl_->connect().isError()) {
            return Result<void>::error("Connecting to NFOV camera failed");
        }

        connected_ = true;

        return Result<void>::success();
    }

    Result<void> Camera::disconnect() {
        if (!connected_) {
            return Result<void>::error("NFOV Camera not connected");
        }

        LOG_INFO("Disconnecting from NFOV camera");
        if (camera_impl_->disconnect().isError()) {
            return Result<void>::error("Connecting to NFOV camera failed");
        }

        connected_ = false;

        return Result<void>::success();
    }

    bool Camera::isConnected() const {
        return connected_;
    }
}