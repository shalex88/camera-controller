#include "SonyCamera.h"

#include <chrono>
#include <utility>

#include "common/Logger/Logger.h"

namespace camera_service::infrastructure {
    SonyCamera::SonyCamera(std::string device_path, std::unique_ptr<Visca> protocol)
        : device_path_(std::move(device_path)), protocol_(std::move(protocol)) {}

    Result<void> SonyCamera::setZoom(const types::zoom zoom) const {
        const auto result = protocol_->setZoomValue(static_cast<uint16_t>(zoom));
        if (result == ErrorCode::Success) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(result));
    }

    Result<types::zoom> SonyCamera::getZoom() const {
        uint16_t value = 0;
        const auto result = protocol_->getZoomValue(&value);

        if (result == ErrorCode::Success) {
            return Result<types::zoom>::success(static_cast<types::zoom>(value));
        }

        return Result<types::zoom>::error("Failed to get zoom value: " + getViscaErrorMessage(result));
    }

    types::ZoomRange SonyCamera::getZoomLimits() const {
        return zoom_limits_;
    }

    Result<void> SonyCamera::setFocus(const types::focus focus) const {
        if (const auto auto_focus_result = isAutoFocusEnabled(); auto_focus_result.isError()) {
            return Result<void>::error("Failed to get focus mode: " + auto_focus_result.error());
        } else if (auto_focus_result.isSuccess()) {
            return Result<void>::error("Cannot set focus value while autofocus is enabled");
        }

        const auto result = protocol_->setFocusValue(static_cast<uint16_t>(focus));
        if (result == ErrorCode::Success) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(result));
    }

    Result<types::focus> SonyCamera::getFocus() const {
        if (const auto auto_focus_result = isAutoFocusEnabled(); auto_focus_result.isError()) {
            return Result<types::focus>::error("Failed to get focus mode: " + auto_focus_result.error());
        } else if (auto_focus_result.isSuccess() && auto_focus_result.value()) {
            return Result<types::focus>::error("Cannot get focus value while autofocus is enabled");
        }

        uint16_t value = 0;
        const auto result = protocol_->getFocusValue(&value);

        if (result == ErrorCode::Success) {
            return Result<types::focus>::success(static_cast<types::focus>(value));
        }

        return Result<types::focus>::error("Failed to get focus value: " + getViscaErrorMessage(result));
    }

    types::FocusRange SonyCamera::getFocusLimits() const {
        return focus_limits_;
    }

    Result<void> SonyCamera::enableAutoFocus(const bool on) const {
        const auto result = protocol_->setFocusAuto(on);
        if (result == ErrorCode::Success) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(result));
    }

    Result<bool> SonyCamera::isAutoFocusEnabled() const {
        bool on = false;
        const auto result = protocol_->getFocusAuto(&on);

        if (result == ErrorCode::Success) {
            return Result<bool>::success(on);
        }

        return Result<bool>::error("Failed to get autofocus status: " + getViscaErrorMessage(result));
    }

    Result<types::info> SonyCamera::getInfo() const {
        if (const auto result = protocol_->getCameraInfo(); result != ErrorCode::Success) { //FIXME: return some string
            return Result<types::info>::error("Failed to get camera info: " + getViscaErrorMessage(result));
        }

        std::string info_str = "Sony VISCA Camera - ";
        info_str += "Vendor: 0x" + std::to_string(camera_.vendor) + ", ";
        info_str += "Model: 0x" + std::to_string(camera_.model) + ", ";
        info_str += "ROM Version: 0x" + std::to_string(camera_.rom_version) + ", ";
        info_str += "Address: " + std::to_string(camera_.address);

        return Result<types::info>::success(info_str);
    }

    Result<void> SonyCamera::stabilize(const bool on) const {
        const auto result = protocol_->setCamStabilizer(on ? VISCA_CAM_STABILIZER_ON : VISCA_CAM_STABILIZER_OFF);
        if (result == ErrorCode::Success) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(result));
    }

    Result<void> SonyCamera::connect() {
        auto result = protocol_->open(device_path_.c_str());
        if (result != ErrorCode::Success) {
            return Result<void>::error("Failed to open serial connection to device: " + device_path_);
        }

        result = protocol_->setAddress();
        if (result != ErrorCode::Success) {
            protocol_->close();
            return Result<void>::error("Failed to set camera address");
        }

        camera_.address = camera_address_;

        result = protocol_->clear();
        if (result != ErrorCode::Success) {
            protocol_->close();
            return Result<void>::error("Failed to clear camera commands");
        }

        result = protocol_->getCameraInfo();
        if (result != ErrorCode::Success) {
            protocol_->close();
            return Result<void>::error("Failed to get camera information");
        }

        return Result<void>::success();
    }

    Result<void> SonyCamera::disconnect() {
        if (const auto result = protocol_->close(); result != ErrorCode::Success) {
            return Result<void>::error("Failed to close serial connection");
        }

        return Result<void>::success();
    }

    std::string SonyCamera::getViscaErrorMessage(const ErrorCode error_code) {
        switch (error_code) {
            case ErrorCode::ErrorMessageLength:
                return "Invalid message length";
            case ErrorCode::ErrorSyntax:
                return "Syntax error";
            case ErrorCode::ErrorCmdBufferFull:
                return "Command buffer full";
            case ErrorCode::ErrorCmdCancelled:
                return "Command cancelled";
            case ErrorCode::ErrorNoSocket:
                return "No socket available";
            case ErrorCode::ErrorCmdNotExecutable:
                return "Command not executable";
            default:
                return "Unknown VISCA error: " + std::to_string(static_cast<uint32_t>(error_code));
        }
    }
}