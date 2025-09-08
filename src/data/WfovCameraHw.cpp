#include "WfovCameraHw.h"

#include <thread>
#include <chrono>

#include "common/Logger/Logger.h"

#define WFOV_CAMERA_LOCK_TIMEOUT_MS 200

namespace camera_service::data {
    WfovCameraHw::~WfovCameraHw() {
        if (disconnect().isError()) {
            LOG_ERROR(disconnect().error());
        }
    }

    Result<void> WfovCameraHw::setZoom(const types::zoom zoom) {
        if (uart_->write(convertToVector(zoom)).isError()) {
            return Result<void>::error("Failed to write value");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(WFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::zoom> WfovCameraHw::getZoom() const {
        const auto zoom = uart_->read();
        if (zoom.isError()) {
            return Result<types::zoom>::error("Failed to read value");
        }
        const types::zoom zoom_int = static_cast<types::zoom>(convertToInt(zoom.value()));
        std::this_thread::sleep_for(std::chrono::milliseconds(WFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::zoom>::success(zoom_int);
    }

    Result<void> WfovCameraHw::setFocus(const types::focus focus) {
        if (uart_->write(convertToVector(focus)).isError()) {
            return Result<void>::error("Failed to write value");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(WFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::focus> WfovCameraHw::getFocus() const {
        const auto focus = uart_->read();
        if (focus.isError())     {
            return Result<types::focus>::error("Failed to read value");
        }
        const types::zoom focus_int = static_cast<types::zoom>(convertToInt(focus.value()));
        std::this_thread::sleep_for(std::chrono::milliseconds(WFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::focus>::success(focus_int);
    }

    Result<void> WfovCameraHw::connect() {
        if (uart_->open().isError()) {
            return Result<void>::error("Failed to connect");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(WFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<void> WfovCameraHw::disconnect() {
        if (uart_->close().isError()) {
            return Result<void>::error("Failed to disconnect");
        }

        return Result<void>::success();
    }

    types::CameraLimits WfovCameraHw::getLimits() const {
        return limits_;
    }

    double WfovCameraHw::convertToInt(const std::vector<char>& data) {
        return std::stod(std::string(data.begin(), data.end()));
    }

    std::vector<char> WfovCameraHw::convertToVector(const double& value) {
        return {std::to_string(value).begin(), std::to_string(value).end()};
    }
}
