#include "CameraHal.h"

namespace camera_service::data {
    CameraHal::CameraHal(std::unique_ptr<ICameraHw> camera_strategy,
                        std::shared_ptr<LayerLogger> logger)
        : camera_hw_(std::move(camera_strategy)), logger_(std::move(logger)) {
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

    Result<void> CameraHal::setZoom(const types::zoom normalized_zoom) const {
        if (!isConnected()) {
            return Result<void>::error(logger_,"Camera not connected");
        }

        const auto* zoom_capable_camera = getCapability<capabilities::IZoomCapable>();
        if (!zoom_capable_camera) {
            return Result<void>::error(logger_, "Camera doesn't support zoom");
        }

        if (!isValidNormalizedZoom(normalized_zoom)) {
            return Result<void>::error(logger_, "Invalid normalized zoom value. Must be 0-100");
        }

        const types::zoom camera_zoom = denormalizeZoom(normalized_zoom);
        if (!isValidCameraZoom(camera_zoom)) {
            return Result<void>::error(logger_, "Invalid zoom value");
        }

        logger_->debug("{} normalized: {}, converted: {}", __func__, normalized_zoom, camera_zoom);
        return zoom_capable_camera->setZoom(camera_zoom);
    }

    Result<types::zoom> CameraHal::getZoom() const {
        if (!isConnected()) {
            return Result<types::zoom>::error(logger_, "Camera not connected");
        }

        const auto* zoom_capable_camera = getCapability<capabilities::IZoomCapable>();
        if (!zoom_capable_camera) {
            return Result<types::zoom>::error(logger_, "Camera doesn't support zoom");
        }

        logger_->debug(__func__);
        const auto zoom_result = zoom_capable_camera->getZoom();

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

    Result<void> CameraHal::setFocus(const types::focus normalized_focus) const {
        if (!isConnected()) {
            return Result<void>::error(logger_, "Camera not connected");
        }

        const auto* focus_capable = getCapability<capabilities::IFocusCapable>();
        if (!focus_capable) {
            return Result<void>::error(logger_, "Camera doesn't support focus");
        }

        if (!isValidNormalizedFocus(normalized_focus)) {
            return Result<void>::error(logger_, "Invalid normalized focus value. Must be 0-100");
        }

        const types::focus camera_focus = denormalizeFocus(normalized_focus);
        if (!isValidCameraFocus(camera_focus)) {
            return Result<void>::error(logger_, "Invalid focus value");
        }

        logger_->debug("{} normalized: {}, converted: {}", __func__, normalized_focus, camera_focus);
        return focus_capable->setFocus(camera_focus);
    }

    Result<types::focus> CameraHal::getFocus() const {
        if (!isConnected()) {
            return Result<types::focus>::error(logger_, "Camera not connected");
        }

        const auto* focus_capable = getCapability<capabilities::IFocusCapable>();
        if (!focus_capable) {
            return Result<types::focus>::error(logger_, "Camera doesn't support focus");
        }

        logger_->debug(__func__);
        const auto focus_result = focus_capable->getFocus();

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

    Result<void> CameraHal::enableAutoFocus(const bool on) const {
        if (!isConnected()) {
            return Result<void>::error(logger_, "Camera not connected");
        }

        const auto* auto_focus_capable = getCapability<capabilities::IAutoFocusCapable>();
        if (!auto_focus_capable) {
            return Result<void>::error(logger_, "Camera doesn't support auto focus");
        }

        logger_->debug(__func__);

        return auto_focus_capable->enableAutoFocus(on);
    }

    Result<types::info> CameraHal::getInfo() const {
        if (!isConnected()) {
            return Result<types::info>::error(logger_, "Camera not connected");
        }

        const auto* info_capable = getCapability<capabilities::IInfoCapable>();
        if (!info_capable) {
            return Result<types::info>::error(logger_, "Camera doesn't support info");
        }

        const auto info_result = info_capable->getInfo();
        if (info_result.isError()) {
            return Result<types::info>::error(logger_, info_result.error());
        }

        logger_->debug("{} {}", __func__, info_result.value());
        return info_result;
    }

    Result<void> CameraHal::stabilize(const bool on) const {
        if (!isConnected()) {
            return Result<void>::error(logger_, "Camera not connected");
        }

        const auto* stabilize_capable = getCapability<capabilities::IStabilizeCapable>();
        if (!stabilize_capable) {
            return Result<void>::error(logger_, "Camera doesn't support auto focus");
        }

        logger_->debug(__func__);

        return stabilize_capable->stabilize(on);
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

    types::ZoomRange CameraHal::getZoomLimits() const {
        const auto* zoom_capable_camera = getCapability<capabilities::IZoomCapable>();
        if (!zoom_capable_camera) {
            throw std::runtime_error("Camera doesn't support zoom"); //FIXME: think of a better solution
        }
        return zoom_capable_camera->getZoomLimits();
    }

    types::FocusRange CameraHal::getFocusLimits() const {
        const auto* focus_capable = getCapability<capabilities::IFocusCapable>();
        if (!focus_capable) {
            throw std::runtime_error("Camera doesn't support focus"); //FIXME: think of a better solution
        }
        return focus_capable->getFocusLimits();
    }

    bool CameraHal::isValidCameraZoom(const types::zoom value) const {
        return value >= getZoomLimits().min && value <= getZoomLimits().max;
    }

    bool CameraHal::isValidCameraFocus(const types::focus value) const {
        return value >= getFocusLimits().min && value <= getFocusLimits().max;
    }

    bool CameraHal::isValidNormalizedZoom(const types::zoom value) {
        return value >= 0 && value <= 100; //FIXME: use constants
    }

    bool CameraHal::isValidNormalizedFocus(const types::focus value) {
        return value >= 0 && value <= 100; //FIXME: use constants
    }

    types::zoom CameraHal::normalizeZoom(const types::zoom camera_zoom) const {
        return (camera_zoom - getZoomLimits().min) * 100 / (getZoomLimits().max - getZoomLimits().min); //FIXME: use constants
    }

    types::focus CameraHal::normalizeFocus(const types::focus camera_focus) const {
        return (camera_focus - getFocusLimits().min) * 100 / (getFocusLimits().max - getFocusLimits().min); //FIXME: use constants
    }

    types::zoom CameraHal::denormalizeZoom(const types::zoom normalized_zoom) const {
        return getZoomLimits().min + (normalized_zoom * (getZoomLimits().max - getZoomLimits().min)) / 100; //FIXME: use constants
    }

    types::focus CameraHal::denormalizeFocus(const types::focus normalized_focus) const {
        return getFocusLimits().min + (normalized_focus * (getFocusLimits().max - getFocusLimits().min)) / 100; //FIXME: use constants
    }
}