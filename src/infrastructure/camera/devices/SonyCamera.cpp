#include "SonyCamera.h"

#include <thread>
#include <chrono>
#include <utility>

#include "common/Logger/Logger.h"

namespace camera_service::infrastructure {
    SonyCamera::SonyCamera(std::string device_path)
        : device_path_(std::move(device_path)) {}

    Result<void> SonyCamera::setZoom(const types::zoom zoom) const {
        const uint32_t result = Visca::setZoomValue(&interface_, &camera_, static_cast<uint16_t>(zoom));
        if (result == VISCA_SUCCESS) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(result));
    }

    Result<types::zoom> SonyCamera::getZoom() const {
        uint16_t value = 0;
        const uint32_t result = Visca::getZoomValue(&interface_, &camera_, &value);

        if (result == VISCA_SUCCESS) {
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
            return Result<void>::error("Cannot set focus value while auto focus is enabled");
        }

        const auto result = Visca::setFocusValue(&interface_, &camera_, static_cast<uint16_t>(focus));
        if (result == VISCA_SUCCESS) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(result));
    }

    Result<types::focus> SonyCamera::getFocus() const {
        if (const auto auto_focus_result = isAutoFocusEnabled(); auto_focus_result.isError()) {
            return Result<types::focus>::error("Failed to get focus mode: " + auto_focus_result.error());
        } else if (auto_focus_result.isSuccess()) {
            return Result<types::focus>::error("Cannot get focus value while auto focus is enabled");
        }

        uint16_t value = 0;
        const uint32_t result = Visca::getFocusValue(&interface_, &camera_, &value);

        if (result == VISCA_SUCCESS) {
            return Result<types::focus>::success(static_cast<types::focus>(value));
        }

        return Result<types::focus>::error("Failed to get focus value: " + getViscaErrorMessage(result));
    }

    types::FocusRange SonyCamera::getFocusLimits() const {
        return focus_limits_;
    }

    Result<void> SonyCamera::enableAutoFocus(const bool on) const {
        const uint32_t result = Visca::setFocusAuto(&interface_, &camera_, on ? VISCA_ON : VISCA_OFF);
        if (result == VISCA_SUCCESS) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(result));
    }

    Result<bool> SonyCamera::isAutoFocusEnabled() const {
        uint8_t value = 0;
        const uint32_t result = Visca::getFocusAuto(&interface_, &camera_, &value);

        if (result == VISCA_SUCCESS) {
            return Result<bool>::success(value == VISCA_ON);
        }

        return Result<bool>::error("Failed to get auto focus status: " + getViscaErrorMessage(result));
    }

    Result<types::info> SonyCamera::getInfo() const {
        const uint32_t result = Visca::getCameraInfo(&interface_, &camera_);
        if (result != VISCA_SUCCESS) {
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
        const uint32_t result = Visca::setCamStabilizer(&interface_, &camera_,
                                                        on ? VISCA_CAM_STABILIZER_ON : VISCA_CAM_STABILIZER_OFF);
        if (result == VISCA_SUCCESS) {
            return Result<void>::success();
        }
        return Result<void>::error(getViscaErrorMessage(result));
    }

    Result<void> SonyCamera::connect() {
        uint32_t result = Visca::openSerial(&interface_, device_path_.c_str());
        if (result != VISCA_SUCCESS) {
            return Result<void>::error("Failed to open serial connection to device: " + device_path_);
        }

        result = Visca::setAddress(&interface_, &camera_address_);
        if (result != VISCA_SUCCESS) {
            Visca::closeSerial(&interface_);
            return Result<void>::error("Failed to set camera address");
        }

        camera_.address = camera_address_;

        result = Visca::clear(&interface_, &camera_);
        if (result != VISCA_SUCCESS) {
            Visca::closeSerial(&interface_);
            return Result<void>::error("Failed to clear camera commands");
        }

        result = Visca::getCameraInfo(&interface_, &camera_);
        if (result != VISCA_SUCCESS) {
            Visca::closeSerial(&interface_);
            return Result<void>::error("Failed to get camera information");
        }

        return Result<void>::success();
    }

    Result<void> SonyCamera::disconnect() {
        const uint32_t result = Visca::closeSerial(&interface_);
        if (result != VISCA_SUCCESS) {
            return Result<void>::error("Failed to close serial connection");
        }

        return Result<void>::success();
    }

    std::string SonyCamera::getViscaErrorMessage(const uint32_t error_code) {
        switch (error_code) {
        case VISCA_ERROR_MESSAGE_LENGTH:
            return "Invalid message length";
        case VISCA_ERROR_SYNTAX:
            return "Syntax error";
        case VISCA_ERROR_CMD_BUFFER_FULL:
            return "Command buffer full";
        case VISCA_ERROR_CMD_CANCELLED:
            return "Command cancelled";
        case VISCA_ERROR_NO_SOCKET:
            return "No socket available";
        case VISCA_ERROR_CMD_NOT_EXECUTABLE:
            return "Command not executable";
        default:
            return "Unknown VISCA error: " + std::to_string(error_code);
        }
    }
}