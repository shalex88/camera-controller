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

    Result<void> CameraHal::setZoom(const types::zoom normalized_zoom) {
        if (!isConnected()) {
            return Result<void>::error(logger_,"Camera not connected");
        }

        if (!isValidNormalizedZoom(normalized_zoom)) {
            return Result<void>::error(logger_, "Invalid normalized zoom value. Must be 0-100");
        }

        const types::zoom camera_zoom = denormalizeZoom(normalized_zoom);
        if (!isValidCameraZoom(camera_zoom)) {
            return Result<void>::error(logger_, "Invalid zoom value");
        }

        logger_->debug("{} normalized: {}, converted: {}", __func__, normalized_zoom, camera_zoom);
        return camera_hw_->setZoom(camera_zoom);
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

        const types::zoom camera_zoom = zoom_result.value();

        if (!isValidCameraZoom(camera_zoom)) {
            return Result<types::zoom>::error(logger_, "Invalid zoom value from camera");
        }

        const types::zoom normalized_zoom = normalizeZoom(camera_zoom);

        logger_->debug("{} camera: {}, normalized: {}", __func__, camera_zoom, normalized_zoom);
        return Result<types::zoom>::success(normalized_zoom);
    }

    Result<void> CameraHal::setFocus(const types::focus normalized_focus) {
        if (!isConnected()) {
            return Result<void>::error(logger_, "Camera not connected");
        }

        if (!isValidNormalizedFocus(normalized_focus)) {
            return Result<void>::error(logger_, "Invalid normalized focus value. Must be 0-100");
        }

        const types::focus camera_focus = denormalizeFocus(normalized_focus);
        if (!isValidCameraFocus(camera_focus)) {
            return Result<void>::error(logger_, "Invalid focus value");
        }

        logger_->debug("{} normalized: {}, converted: {}", __func__, normalized_focus, camera_focus);
        return camera_hw_->setFocus(camera_focus);
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

        const types::focus camera_focus = focus_result.value();

        if (!isValidCameraFocus(camera_focus)) {
            return Result<types::focus>::error(logger_, "Invalid focus value from camera");
        }

        const types::focus normalized_focus = normalizeFocus(camera_focus);

        logger_->debug("{} camera: {}, normalized: {}", __func__, camera_focus, normalized_focus);
        return Result<types::focus>::success(normalized_focus);
    }

    Result<types::info> CameraHal::getInfo() const {
        if (!isConnected()) {
            return Result<types::info>::error(logger_, "Camera not connected");
        }

        logger_->debug(__func__);
        auto info_result = camera_hw_->getInfo();

        if (info_result.isError()) {
            return Result<types::info>::error(logger_, info_result.error());
        }

        logger_->debug("{} {}", __func__, info_result.value());
        return info_result;
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

    bool CameraHal::isValidCameraZoom(const types::zoom value) const {
        return value >= limits_.min_zoom && value <= limits_.max_zoom;
    }

    bool CameraHal::isValidCameraFocus(const types::focus value) const {
        return value >= limits_.min_focus && value <= limits_.max_focus;
    }

    bool CameraHal::isValidNormalizedZoom(const types::zoom value) {
        return value >= 0 && value <= 100;
    }

    bool CameraHal::isValidNormalizedFocus(const types::focus value) {
        return value >= 0 && value <= 100;
    }

    types::zoom CameraHal::normalizeZoom(const types::zoom camera_zoom) const {
        return (camera_zoom - limits_.min_zoom) * 100 / (limits_.max_zoom - limits_.min_zoom);
    }

    types::focus CameraHal::normalizeFocus(const types::focus camera_focus) const {
        return (camera_focus - limits_.min_focus) * 100 / (limits_.max_focus - limits_.min_focus);
    }

    types::zoom CameraHal::denormalizeZoom(const types::zoom normalized_zoom) const {
        return limits_.min_zoom + (normalized_zoom * (limits_.max_zoom - limits_.min_zoom)) / 100;
    }

    types::focus CameraHal::denormalizeFocus(const types::focus normalized_focus) const {
        return limits_.min_focus + (normalized_focus * (limits_.max_focus - limits_.min_focus)) / 100;
    }
}
