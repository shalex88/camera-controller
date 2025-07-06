#include "Camera.h"

#include <iostream>

#include "common/Logger/Logger.h"

namespace camera_service::data {
    Camera::Camera(std::unique_ptr<ICameraStrategy> camera_strategy) : camera_impl_(std::move(camera_strategy)), limits_(camera_impl_->getLimits()) {
    }

    Camera::~Camera() {
        if (connected_) {
            disconnect();
        }
    }

    Result<void> Camera::setZoom(const types::zoom zoom) {
        if (!connected_) {
            return Result<void>::error("Cannot set zoom: NFOV Camera not connected");
        }

        if (!isValidZoom(zoom)) {
            return Result<void>::error("Invalid zoom value");
        }

        LOG_INFO("Setting camera zoom to: {}", zoom);
        return camera_impl_->setZoom(zoom);
    }

    Result<types::zoom> Camera::getZoom() const {
        if (!connected_) {
            return Result<types::zoom>::error("Cannot get zoom: NFOV Camera not connected");
        }

        LOG_INFO("Getting camera zoom");
        auto zoom_result = camera_impl_->getZoom();

        if (zoom_result.isError()) {
            return Result<types::zoom>::error("Failed to get zoom: " + zoom_result.error());
        }

        if (!isValidFocus(zoom_result.value())) {
            return Result<types::zoom>::error("Invalid zoom value");
        }

        return zoom_result;
    }

    Result<void> Camera::setFocus(const types::focus focus) {
        if (!connected_) {
            return Result<void>::error("Cannot set focus: NFOV Camera not connected");
        }

        if (!isValidFocus(focus)) {
            return Result<void>::error("Invalid focus value");
        }

        LOG_INFO("Setting camera focus to: {}", focus);
        return camera_impl_->setFocus(focus);
    }

    Result<types::focus> Camera::getFocus() const {
        if (!connected_) {
            return Result<types::focus>::error("Cannot get focus: NFOV Camera not connected");
        }
        LOG_INFO("Getting camera focus");
        auto focus_result = camera_impl_->getFocus();

        if (focus_result.isError()) {
            return Result<types::zoom>::error("Failed to get focus: " + focus_result.error());
        }

        if (!isValidFocus(focus_result.value())) {
            return Result<types::zoom>::error("Invalid focus value");
        }

        return focus_result;
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

    bool Camera::isValidZoom(types::zoom value) const {
        return value >= limits_.min_zoom && value <= limits_.max_zoom;
    }

    bool Camera::isValidFocus(types::focus value) const {
        return value >= limits_.min_focus && value <= limits_.max_focus;
    }
}