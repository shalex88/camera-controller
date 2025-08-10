#include "CameraHal.h"

#include "common/Logger/Logger.h"

namespace camera_service::data {
    CameraHal::CameraHal(std::unique_ptr<ICameraHw> camera_strategy) : camera_hw_(std::move(camera_strategy)), limits_(camera_hw_->getLimits()) {
    }

    CameraHal::~CameraHal() {
        if (connected_) {
            if (disconnect().isError()) {
                LOG_ERROR("[DAL] Failed to disconnect Camera");
            }
        }
    }

    Result<void> CameraHal::setZoom(const types::zoom zoom) {
        if (!connected_) {
            return Result<void>::error("[DAL] Camera not connected");
        }

        if (!isValidZoom(zoom)) {
            return Result<void>::error("[DAL] Invalid zoom value");
        }

        LOG_INFO("[DAL] Setting camera zoom to: {}", zoom);
        return camera_hw_->setZoom(zoom);
    }

    Result<types::zoom> CameraHal::getZoom() const {
        if (!connected_) {
            return Result<types::zoom>::error("[DAL] Camera not connected");
        }

        LOG_INFO("[DAL] Getting camera zoom");
        auto zoom_result = camera_hw_->getZoom();

        if (zoom_result.isError()) {
            return Result<types::zoom>::error("[DAL] Failed to get zoom: " + zoom_result.error());
        }

        if (!isValidFocus(zoom_result.value())) {
            return Result<types::zoom>::error("[DAL] Invalid zoom value");
        }

        LOG_INFO("[DAL] Current Zoom is: {}", zoom_result.value());

        return zoom_result;
    }

    Result<void> CameraHal::setFocus(const types::focus focus) {
        if (!connected_) {
            return Result<void>::error("[DAL] Camera not connected");
        }

        if (!isValidFocus(focus)) {
            return Result<void>::error("[DAL] Invalid focus value");
        }

        LOG_INFO("[DAL] Setting camera focus to: {}", focus);
        return camera_hw_->setFocus(focus);
    }

    Result<types::focus> CameraHal::getFocus() const {
        if (!connected_) {
            return Result<types::focus>::error("[DAL] Camera not connected");
        }
        LOG_INFO("[DAL] Getting camera focus");
        auto focus_result = camera_hw_->getFocus();

        if (focus_result.isError()) {
            return Result<types::zoom>::error("[DAL] Failed to get focus: " + focus_result.error());
        }

        if (!isValidFocus(focus_result.value())) {
            return Result<types::zoom>::error("[DAL] Invalid focus value");
        }

        LOG_INFO("[DAL] Current Focus is: {}", focus_result.value());

        return focus_result;
    }

    Result<void> CameraHal::connect() {
        if (connected_) {
            return Result<void>::error("[DAL] Camera already connected");
        }

        LOG_INFO("[DAL] Connecting to camera");
        if (camera_hw_->connect().isError()) {
            return Result<void>::error("[DAL] Connecting to camera failed");
        }

        connected_ = true;

        return Result<void>::success();
    }

    Result<void> CameraHal::disconnect() {
        if (!connected_) {
            return Result<void>::error("[DAL] Camera not connected");
        }

        LOG_INFO("[DAL] Disconnecting from camera");
        if (camera_hw_->disconnect().isError()) {
            return Result<void>::error("[DAL] Connecting to camera failed");
        }

        connected_ = false;

        return Result<void>::success();
    }

    bool CameraHal::isConnected() const {
        return connected_;
    }

    bool CameraHal::isValidZoom(const types::zoom value) const {
        return value >= limits_.min_zoom && value <= limits_.max_zoom;
    }

    bool CameraHal::isValidFocus(const types::focus value) const {
        return value >= limits_.min_focus && value <= limits_.max_focus;
    }
}