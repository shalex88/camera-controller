#include "Camera.h"

#include "common/logger/Logger.h"
#include "infrastructure/camera/hal/ICameraHw.h"

namespace camera_service::infrastructure {
    Camera::Camera(std::unique_ptr<ICameraHw> camera_strategy)
        : camera_hw_(std::move(camera_strategy)) {
        if (!camera_hw_) {
            throw std::invalid_argument("Camera hardware cannot be null");
        }
        if (getZoomLimits().min >= getZoomLimits().max) {
            throw std::runtime_error("Invalid zoom limits from camera");
        }
        if (getFocusLimits().min >= getFocusLimits().max) {
            throw std::runtime_error("Invalid focus limits from camera");
        }
    }

    Camera::~Camera() {
        if (isConnected()) {
            if (disconnect().isError()) {
                LOG_ERROR("Failed to disconnect Camera");
            }
        }
    }

    Result<void> Camera::setZoom(const types::zoom normalized_zoom) const {
        if (!isConnected()) {
            return Result<void>::error("Camera not connected");
        }

        const auto* zoom_capable_camera = getCapability<capabilities::IZoomCapable>();
        if (!zoom_capable_camera) {
            return Result<void>::error("Camera doesn't support zoom");
        }

        if (!isValidNormalizedZoom(normalized_zoom)) {
            return Result<void>::error("Invalid normalized zoom value. Must be 0-100");
        }

        const types::zoom camera_zoom = denormalizeZoom(normalized_zoom);
        if (!isValidCameraZoom(camera_zoom)) {
            return Result<void>::error("Invalid zoom value");
        }

        LOG_DEBUG("{} normalized: {}, converted: {}", __func__, normalized_zoom, camera_zoom);
        return zoom_capable_camera->setZoom(camera_zoom);
    }

    Result<types::zoom> Camera::getZoom() const {
        if (!isConnected()) {
            return Result<types::zoom>::error("Camera not connected");
        }

        const auto* zoom_capable_camera = getCapability<capabilities::IZoomCapable>();
        if (!zoom_capable_camera) {
            return Result<types::zoom>::error("Camera doesn't support zoom");
        }

        LOG_DEBUG(__func__);
        const auto zoom_result = zoom_capable_camera->getZoom();

        if (zoom_result.isError()) {
            return Result<types::zoom>::error(zoom_result.error());
        }

        const types::zoom camera_zoom = zoom_result.value();

        if (!isValidCameraZoom(camera_zoom)) {
            return Result<types::zoom>::error("Invalid zoom value from camera");
        }

        const types::zoom normalized_zoom = normalizeZoom(camera_zoom);

        LOG_DEBUG("{} camera: {}, normalized: {}", __func__, camera_zoom, normalized_zoom);
        return Result<types::zoom>::success(normalized_zoom);
    }

    Result<void> Camera::setFocus(const types::focus normalized_focus) const {
        if (!isConnected()) {
            return Result<void>::error("Camera not connected");
        }

        const auto* focus_capable = getCapability<capabilities::IFocusCapable>();
        if (!focus_capable) {
            return Result<void>::error("Camera doesn't support focus");
        }

        if (!isValidNormalizedFocus(normalized_focus)) {
            return Result<void>::error("Invalid normalized focus value. Must be 0-100");
        }

        const types::focus camera_focus = denormalizeFocus(normalized_focus);
        if (!isValidCameraFocus(camera_focus)) {
            return Result<void>::error("Invalid focus value");
        }

        LOG_DEBUG("{} normalized: {}, converted: {}", __func__, normalized_focus, camera_focus);
        return focus_capable->setFocus(camera_focus);
    }

    Result<types::focus> Camera::getFocus() const {
        if (!isConnected()) {
            return Result<types::focus>::error("Camera not connected");
        }

        const auto* focus_capable = getCapability<capabilities::IFocusCapable>();
        if (!focus_capable) {
            return Result<types::focus>::error("Camera doesn't support focus");
        }

        LOG_DEBUG(__func__);
        const auto focus_result = focus_capable->getFocus();

        if (focus_result.isError()) {
            return Result<types::focus>::error(focus_result.error());
        }

        const types::focus camera_focus = focus_result.value();

        if (!isValidCameraFocus(camera_focus)) {
            return Result<types::focus>::error("Invalid focus value from camera");
        }

        const types::focus normalized_focus = normalizeFocus(camera_focus);

        LOG_DEBUG("{} camera: {}, normalized: {}", __func__, camera_focus, normalized_focus);
        return Result<types::focus>::success(normalized_focus);
    }

    Result<void> Camera::enableAutoFocus(const bool on) const {
        if (!isConnected()) {
            return Result<void>::error("Camera not connected");
        }

        const auto* auto_focus_capable = getCapability<capabilities::IAutoFocusCapable>();
        if (!auto_focus_capable) {
            return Result<void>::error("Camera doesn't support autofocus");
        }

        LOG_DEBUG(__func__);

        return auto_focus_capable->enableAutoFocus(on);
    }

    Result<types::info> Camera::getInfo() const {
        if (!isConnected()) {
            return Result<types::info>::error("Camera not connected");
        }

        const auto* info_capable = getCapability<capabilities::IInfoCapable>();
        if (!info_capable) {
            return Result<types::info>::error("Camera doesn't support info");
        }

        auto info_result = info_capable->getInfo();
        if (info_result.isError()) {
            return Result<types::info>::error(info_result.error());
        }

        LOG_DEBUG("{} {}", __func__, info_result.value());
        return info_result;
    }

    Result<void> Camera::stabilize(const bool on) const {
        if (!isConnected()) {
            return Result<void>::error("Camera not connected");
        }

        const auto* stabilize_capable = getCapability<capabilities::IStabilizeCapable>();
        if (!stabilize_capable) {
            return Result<void>::error("Camera doesn't support stabilization");
        }

        LOG_DEBUG(__func__);

        return stabilize_capable->stabilize(on);
    }

    Result<void> Camera::connect() {
        if (connected_) {
            return Result<void>::error("Camera already connected");
        }

        LOG_INFO("Connecting to camera...");
        if (const auto connect_result = camera_hw_->connect(); connect_result.isError()) {
            return Result<void>::error(connect_result.error());
        }

        connected_ = true;
        LOG_INFO("Camera connected successfully");
        return Result<void>::success();
    }

    Result<void> Camera::disconnect() {
        if (!connected_) {
            return Result<void>::error("Camera not connected");
        }

        LOG_DEBUG(__func__);
        if (const auto disconnect_result = camera_hw_->disconnect(); disconnect_result.isError()) {
            return Result<void>::error(disconnect_result.error());
        }

        connected_ = false;
        return Result<void>::success();
    }

    bool Camera::isConnected() const {
        return connected_;
    }

    types::ZoomRange Camera::getZoomLimits() const {
        const auto* zoom_capable_camera = getCapability<capabilities::IZoomCapable>();
        if (!zoom_capable_camera) {
            throw std::runtime_error("Camera doesn't support zoom"); //FIXME: think of a better solution
        }
        return zoom_capable_camera->getZoomLimits();
    }

    types::FocusRange Camera::getFocusLimits() const {
        const auto* focus_capable_camera = getCapability<capabilities::IFocusCapable>();
        if (!focus_capable_camera) {
            throw std::runtime_error("Camera doesn't support focus"); //FIXME: think of a better solution
        }
        return focus_capable_camera->getFocusLimits();
    }

    bool Camera::isValidCameraZoom(const types::zoom value) const {
        return value >= getZoomLimits().min && value <= getZoomLimits().max;
    }

    bool Camera::isValidCameraFocus(const types::focus value) const {
        return value >= getFocusLimits().min && value <= getFocusLimits().max;
    }

    bool Camera::isValidNormalizedZoom(const types::zoom value) {
        return value >= types::MIN_NORMALIZED_ZOOM && value <= types::MAX_NORMALIZED_ZOOM;
    }

    bool Camera::isValidNormalizedFocus(const types::focus value) {
        return value >= types::MIN_NORMALIZED_FOCUS && value <= types::MAX_NORMALIZED_FOCUS;
    }

    types::zoom Camera::normalizeZoom(const types::zoom camera_zoom) const {
        const auto [min, max] = getZoomLimits();
        const auto range = max - min;

        const long long numerator = static_cast<long long>(camera_zoom - min) * types::MAX_NORMALIZED_ZOOM;
        const long long rounded = (numerator >= 0)
            ? (numerator + range / 2) / range
            : -(((-numerator) + range / 2) / range);

        return static_cast<types::zoom>(rounded);
    }

    types::focus Camera::normalizeFocus(const types::focus camera_focus) const {
        const auto [min, max] = getFocusLimits();
        const auto range = max - min;

        const long long numerator = static_cast<long long>(camera_focus - min) * types::MAX_NORMALIZED_FOCUS;
        const long long rounded = (numerator >= 0)
            ? (numerator + range / 2) / range
            : -(((-numerator) + range / 2) / range);

        return static_cast<types::focus>(rounded);
    }

    types::zoom Camera::denormalizeZoom(const types::zoom normalized_zoom) const {
        const auto [min, max] = getZoomLimits();
        const auto range = max - min;

        const long long numerator = static_cast<long long>(normalized_zoom) * range;
        const long long rounded = (numerator >= 0)
            ? (numerator + types::MAX_NORMALIZED_ZOOM / 2) / types::MAX_NORMALIZED_ZOOM
            : -(((-numerator) + types::MAX_NORMALIZED_ZOOM / 2) / types::MAX_NORMALIZED_ZOOM);

        return static_cast<types::zoom>(min + rounded);
    }

    types::focus Camera::denormalizeFocus(const types::focus normalized_focus) const {
        const auto [min, max] = getFocusLimits();
        const auto range = max - min;

        const long long numerator = static_cast<long long>(normalized_focus) * range;
        const long long rounded = (numerator >= 0)
            ? (numerator + types::MAX_NORMALIZED_FOCUS / 2) / types::MAX_NORMALIZED_FOCUS
            : -(((-numerator) + types::MAX_NORMALIZED_FOCUS / 2) / types::MAX_NORMALIZED_FOCUS);

        return static_cast<types::focus>(min + rounded);
    }

    template <typename Capability>
    Capability* Camera::getCapability() const {
        return dynamic_cast<Capability*>(camera_hw_.get());
    }
}