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
        if (isConnected()) {
            if (disconnect().isError()) {
                logger_->error("Failed to disconnect Camera");
            }
        }
    }

    Result<void> CameraHal::setZoom(const types::zoom zoom) {
        if (!isConnected()) {
            return Result<void>::error(logger_,"Camera not connected");
        }

        if (!isValidZoom(zoom)) {
            return Result<void>::error(logger_, "Invalid zoom value");
        }

        logger_->debug("{} {}", __func__, zoom);
        return camera_hw_->setZoom(zoom);
    }

    Result<types::zoom> CameraHal::getZoom() const {
        if (!isConnected()) {
            return Result<types::zoom>::error(logger_, "Camera not connected");
        }

        logger_->debug(__func__);
        auto zoom_result = camera_hw_->getZoom();

        if (zoom_result.isError()) {
            return Result<types::zoom>::error(logger_, zoom_result.error());
        }

        if (!isValidZoom(zoom_result.value())) {
            return Result<types::zoom>::error(logger_, "Invalid zoom value");
        }

        logger_->debug("{} {}", __func__, zoom_result.value());
        return zoom_result;
    }

    Result<void> CameraHal::setFocus(const types::focus focus) {
        if (!isConnected()) {
            return Result<void>::error(logger_, "Camera not connected");
        }

        if (!isValidFocus(focus)) {
            return Result<void>::error(logger_, "Invalid focus value");
        }

        logger_->debug("{} {}", __func__, focus);
        return camera_hw_->setFocus(focus);
    }

    Result<types::focus> CameraHal::getFocus() const {
        if (!isConnected()) {
            return Result<types::focus>::error(logger_, "Camera not connected");
        }

        logger_->debug(__func__);
        auto focus_result = camera_hw_->getFocus();

        if (focus_result.isError()) {
            return Result<types::focus>::error(logger_, focus_result.error());
        }

        if (!isValidFocus(focus_result.value())) {
            return Result<types::focus>::error(logger_, "Invalid focus value");
        }

        logger_->debug("{} {}", __func__, focus_result.value());
        return focus_result;
    }

    Result<void> CameraHal::connect() {
        if (connected_) {
            return Result<void>::error(logger_, "Camera already connected");
        }

        logger_->info("Connecting to camera...");
        const auto connect_result = camera_hw_->connect();
        if (connect_result.isError()) {
            return Result<void>::error(logger_, connect_result.error());
        }

        connected_ = true;
        logger_->info("Camera connected successfully");
        return Result<void>::success();
    }

    Result<void> CameraHal::disconnect() {
        if (!connected_) {
            return Result<void>::error(logger_, "Camera not connected");
        }

        logger_->debug(__func__);
        const auto disconnect_result = camera_hw_->disconnect();
        if (disconnect_result.isError()) {
            return Result<void>::error(logger_, disconnect_result.error());
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