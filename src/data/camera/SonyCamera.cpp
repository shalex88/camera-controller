#include "SonyCamera.h"

#include <thread>
#include <chrono>

#include "common/Logger/Logger.h"

#define WFOV_CAMERA_LOCK_TIMEOUT_MS 200

namespace camera_service::data {
    SonyCamera::~SonyCamera() {
        if (disconnect().isError()) {
            LOG_ERROR(disconnect().error());
        }
    }

    Result<void> SonyCamera::setZoom(const types::zoom zoom) {
        if (uart_->write(convertToVector(zoom)).isError()) {
            return Result<void>::error("Failed to write value");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(WFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::zoom> SonyCamera::getZoom() const {
        std::vector<char> getZoomCmd = {'G', 'Z'};
        auto writeResult = uart_->write(getZoomCmd);
        if (writeResult.isError()) {
            return Result<types::zoom>::error("Failed to send get zoom request: " + writeResult.error());
        }

        const auto zoom = uart_->read(); // TODO: Blocking?
        if (zoom.isError()) {
            return Result<types::zoom>::error("Failed to read zoom value: " + zoom.error());
        }

        const types::zoom zoom_int = static_cast<types::zoom>(convertToInt(zoom.value()));
        return Result<types::zoom>::success(zoom_int);
    }

    Result<void> SonyCamera::setFocus(const types::focus focus) {
        if (uart_->write(convertToVector(focus)).isError()) {
            return Result<void>::error("Failed to write value");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(WFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::focus> SonyCamera::getFocus() const {
        std::vector<char> getFocusCmd = {'G', 'F'};
        auto writeResult = uart_->write(getFocusCmd);
        if (writeResult.isError()) {
            return Result<types::focus>::error("Failed to send get focus request: " + writeResult.error());
        }

        const auto focus = uart_->read(); // TODO: Blocking?
        if (focus.isError()) {
            return Result<types::focus>::error("Failed to read focus value: " + focus.error());
        }

        const types::focus focus_int = static_cast<types::focus>(convertToInt(focus.value()));
        return Result<types::focus>::success(focus_int);
    }

    Result<void> SonyCamera::connect() {
        if (uart_->open().isError()) {
            return Result<void>::error("Failed to connect to device");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(WFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<void> SonyCamera::disconnect() {
        if (uart_->close().isError()) {
            return Result<void>::error("Failed to disconnect");
        }

        return Result<void>::success();
    }

    types::CameraLimits SonyCamera::getLimits() const {
        return limits_;
    }

    double SonyCamera::convertToInt(const std::vector<char>& data) {
        return std::stod(std::string(data.begin(), data.end()));
    }

    std::vector<char> SonyCamera::convertToVector(const double& value) {
        return {std::to_string(value).begin(), std::to_string(value).end()};
    }
}
