#include "SonyCamera.h"

#include <chrono>
#include <utility>

#include "common/Logger/Logger.h"
#include "infrastructure/camera/transport/visca/Visca.h"

namespace camera_service::infrastructure {
    SonyCamera::SonyCamera(std::unique_ptr<Visca> protocol)
        : protocol_(std::move(protocol)) {}

    Result<void> SonyCamera::setZoom(const types::zoom zoom) const {
        const auto result = protocol_->setZoomValue(static_cast<uint16_t>(zoom));
        if (result == ErrorCode::Success) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(static_cast<uint32_t>(result)));
    }

    Result<types::zoom> SonyCamera::getZoom() const {
        uint16_t value = 0;
        const auto result = protocol_->getZoomValue(&value);

        if (result == ErrorCode::Success) {
            return Result<types::zoom>::success(static_cast<types::zoom>(value));
        }

        return Result<types::zoom>::error("Failed to get zoom value: " + getViscaErrorMessage(static_cast<uint32_t>(result)));
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
        return Result<void>::error(getViscaErrorMessage(static_cast<uint32_t>(result)));
    }

    Result<types::focus> SonyCamera::getFocus() const {
        if (const auto auto_focus_result = isAutoFocusEnabled(); auto_focus_result.isError()) {
            return Result<types::focus>::error("Failed to get focus mode: " + auto_focus_result.error());
        } else if (auto_focus_result.isSuccess()) {
            return Result<types::focus>::error("Cannot get focus value while autofocus is enabled");
        }

        uint16_t value = 0;
        const auto result = protocol_->getFocusValue(&value);

        if (result == ErrorCode::Success) {
            return Result<types::focus>::success(static_cast<types::focus>(value));
        }

        return Result<types::focus>::error("Failed to get focus value: " + getViscaErrorMessage(static_cast<uint32_t>(result)));
    }

    types::FocusRange SonyCamera::getFocusLimits() const {
        return focus_limits_;
    }

    Result<void> SonyCamera::enableAutoFocus(const bool on) const {
        const auto result = protocol_->setFocusAuto(on ? VISCA_ON : VISCA_OFF);
        if (result == ErrorCode::Success) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(static_cast<uint32_t>(result)));
    }

    Result<bool> SonyCamera::isAutoFocusEnabled() const {
        uint8_t value = 0;
        const auto result = protocol_->getFocusAuto(&value);
        if (result == ErrorCode::Success) {
            return Result<bool>::success(value == VISCA_ON);
        }

        return Result<bool>::error("Failed to get autofocus status: " + getViscaErrorMessage(static_cast<uint32_t>(result)));
    }

    Result<types::info> SonyCamera::getInfo() const {
        const auto result = protocol_->getCameraInfo(&camera_);
        if (result != ErrorCode::Success) {
            return Result<types::info>::error("Failed to get camera info: " + getViscaErrorMessage(static_cast<uint32_t>(result)));
        }

        std::string info_str = "Sony VISCA Camera - ";
        info_str += "Vendor: 0x" + std::to_string(camera_.vendor) + ", ";
        info_str += "Model: 0x" + std::to_string(camera_.model) + ", ";
        info_str += "ROM Version: 0x" + std::to_string(camera_.rom_version) + ", ";
        info_str += "Address: " + std::to_string(camera_.address);

        return Result<types::info>::success(info_str);
    }

    Result<void> SonyCamera::stabilize(const bool on) const {
        const auto result = protocol_->setCamStabilizer(
                                                        on ? VISCA_CAM_STABILIZER_ON : VISCA_CAM_STABILIZER_OFF);
        if (result == ErrorCode::Success) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(static_cast<uint32_t>(result)));
    }

    Result<void> SonyCamera::connect() {
        if (protocol_->connect() != ErrorCode::Success) {
            return Result<void>::error("Failed to open connection");
        }

        return Result<void>::success();
    }

    Result<void> SonyCamera::disconnect() {
        if (protocol_->disconnect() != ErrorCode::Success) {
            return Result<void>::error("Failed to close connection");
        }

        return Result<void>::success();
    }

    std::string SonyCamera::getViscaErrorMessage(const uint32_t error_code) {
        // Use const references for ErrorCode enum values
        const auto error = static_cast<ErrorCode>(error_code);

        if (error == ErrorCode::ErrorMessageLength) {
            return "Invalid message length";
        }
        if (error == ErrorCode::ErrorSyntax) {
            return "Syntax error";
        }
        if (error == ErrorCode::ErrorCmdBufferFull) {
            return "Command buffer full";
        }
        if (error == ErrorCode::ErrorCmdCancelled) {
            return "Command cancelled";
        }
        if (error == ErrorCode::ErrorNoSocket) {
            return "No socket available";
        }
        if (error == ErrorCode::ErrorCmdNotExecutable) {
            return "Command not executable";
        }
        return "Unknown VISCA error: " + std::to_string(error_code);
    }
}