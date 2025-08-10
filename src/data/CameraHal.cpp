#include "CameraHal.h"

namespace camera_service::data {
    CameraHal::CameraHal(std::unique_ptr<ICameraHw> camera_strategy,
                        std::shared_ptr<LayerLogger> logger)
        : camera_hw_(std::move(camera_strategy)), logger_(std::move(logger)), limits_(camera_hw_->getLimits()) {
        if (!camera_hw_) {
            throw std::invalid_argument("Camera hardware cannot be null");
        }
        if (!logger_) {
            throw std::invalid_argument("Logger cannot be null");
        }
    }

    CameraHal::~CameraHal() {
        if (connected_) {
            if (disconnect().isError()) {
                logger_->error("Failed to disconnect Camera");
            }
        }
    }

    Result<void> CameraHal::setZoom(const types::zoom zoom) {
        if (!connected_) {
            return Result<void>::error("Camera not connected");
        }

        if (!isValidZoom(zoom)) {
            return Result<void>::error("Invalid zoom value");
        }

        logger_->info("Setting camera zoom to: {}", zoom);
        return camera_hw_->setZoom(zoom);
    }

    Result<types::zoom> CameraHal::getZoom() const {
        if (!connected_) {
            return Result<types::zoom>::error("Camera not connected");
        }

        logger_->info("Getting camera zoom");
        auto zoom_result = camera_hw_->getZoom();

        if (zoom_result.isError()) {
            return Result<types::zoom>::error("Failed to get zoom: " + zoom_result.error());
        }

        if (!isValidZoom(zoom_result.value())) {
            return Result<types::zoom>::error("Invalid zoom value");
        }

        logger_->info("Current Zoom is: {}", zoom_result.value());
        return zoom_result;
    }

    Result<void> CameraHal::setFocus(const types::focus focus) {
        if (!connected_) {
            return Result<void>::error("Camera not connected");
        }

        if (!isValidFocus(focus)) {
            return Result<void>::error("Invalid focus value");
        }

        logger_->info("Setting camera focus to: {}", focus);
        return camera_hw_->setFocus(focus);
    }

    Result<types::focus> CameraHal::getFocus() const {
        if (!connected_) {
            return Result<types::focus>::error("Camera not connected");
        }

        logger_->info("Getting camera focus");
        auto focus_result = camera_hw_->getFocus();

        if (focus_result.isError()) {
            return Result<types::focus>::error("Failed to get focus: " + focus_result.error());
        }

        if (!isValidFocus(focus_result.value())) {
            return Result<types::focus>::error("Invalid focus value");
        }

        logger_->info("Current Focus is: {}", focus_result.value());
        return focus_result;
    }

    Result<void> CameraHal::connect() {
        if (connected_) {
            return Result<void>::error("Camera already connected");
        }

        logger_->info("Connecting to camera");
        if (camera_hw_->connect().isError()) {
            return Result<void>::error("Connecting to camera failed");
        }

        connected_ = true;
        return Result<void>::success();
    }

    Result<void> CameraHal::disconnect() {
        if (!connected_) {
            return Result<void>::error("Camera not connected");
        }

        logger_->info("Disconnecting from camera");
        if (camera_hw_->disconnect().isError()) {
            return Result<void>::error("Disconnecting from camera failed");
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