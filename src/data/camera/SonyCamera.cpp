#include "SonyCamera.h"

#include <thread>
#include <chrono>
#include <utility>

#include "common/Logger/Logger.h"

namespace camera_service::data {
    SonyCamera::SonyCamera(std::string device_path)
        : device_path_(std::move(device_path)) {
    }

    Result<void> SonyCamera::setZoom(const types::zoom zoom) const {
        return sendCommand([this, zoom]() {
            return VISCA_set_zoom_value(&interface_, &camera_, static_cast<uint16_t>(zoom));
        });
    }

    Result<types::zoom> SonyCamera::getZoom() const {
        const auto result = sendInquiry([this](uint16_t* value) {
            return VISCA_get_zoom_value(&interface_, &camera_, value);
        });

        if (result.isError()) {
            return Result<types::zoom>::error("Failed to get zoom value: " + result.error());
        }

        return Result<types::zoom>::success(static_cast<types::zoom>(result.value()));
    }

    Result<void> SonyCamera::setFocus(const types::focus focus) const {
        const auto manual_result = sendCommand([this]() {
            return VISCA_set_focus_auto(&interface_, &camera_, VISCA_OFF);
        });

        if (manual_result.isError()) {
            return Result<void>::error("Failed to set manual focus mode: " + manual_result.error());
        }

        return sendCommand([this, focus]() {
            return VISCA_set_focus_value(&interface_, &camera_, static_cast<uint16_t>(focus));
        });
    }

    Result<types::focus> SonyCamera::getFocus() const {
        const auto result = sendInquiry([this](uint16_t* value) {
            return VISCA_get_focus_value(&interface_, &camera_, value);
        });

        if (result.isError()) {
            return Result<types::focus>::error("Failed to get focus value: " + result.error());
        }

        return Result<types::focus>::success(static_cast<types::focus>(result.value()));
    }

    Result<types::info> SonyCamera::getInfo() const {
        const auto info_result = sendCommand([this]() {
            return VISCA_get_camera_info(&interface_, &camera_);
        });

        if (info_result.isError()) {
            return Result<types::info>::error("Failed to get camera info: " + info_result.error());
        }

        std::string info_str = "Sony VISCA Camera - ";
        info_str += "Vendor: 0x" + std::to_string(camera_.vendor) + ", ";
        info_str += "Model: 0x" + std::to_string(camera_.model) + ", ";
        info_str += "ROM Version: 0x" + std::to_string(camera_.rom_version) + ", ";
        info_str += "Address: " + std::to_string(camera_.address);

        return Result<types::info>::success(info_str);
    }

    Result<void> SonyCamera::connect() {
        uint32_t result = VISCA_open_serial(&interface_, device_path_.c_str());
        if (result != VISCA_SUCCESS) {
            return Result<void>::error("Failed to open serial connection to device: " + device_path_);
        }

        result = VISCA_set_address(&interface_, &camera_address_);
        if (result != VISCA_SUCCESS) {
            VISCA_close_serial(&interface_);
            return Result<void>::error("Failed to set camera address");
        }

        camera_.address = camera_address_;

        result = VISCA_clear(&interface_, &camera_);
        if (result != VISCA_SUCCESS) {
            VISCA_close_serial(&interface_);
            return Result<void>::error("Failed to clear camera commands");
        }

        result = VISCA_get_camera_info(&interface_, &camera_);
        if (result != VISCA_SUCCESS) {
            VISCA_close_serial(&interface_);
            return Result<void>::error("Failed to get camera information");
        }

        return Result<void>::success();
    }

    Result<void> SonyCamera::disconnect() {
        const uint32_t result = VISCA_close_serial(&interface_);
        if (result != VISCA_SUCCESS) {
            return Result<void>::error("Failed to close serial connection");
        }

        return Result<void>::success();
    }

    types::CameraLimits SonyCamera::getLimits() const {
        return limits_;
    }

    Result<void> SonyCamera::setMinZoom() const {
        return setZoom(limits_.min_zoom);
    }

    Result<void> SonyCamera::setMaxZoom() const {
        return setZoom(limits_.max_zoom);
    }

    Result<void> SonyCamera::sendCommand(const std::function<uint32_t()>& command) {
        const uint32_t result = command();
        if (result == VISCA_SUCCESS) {
            return Result<void>::success();
        }

        std::string error_msg;
        switch (result) {
            case VISCA_ERROR_MESSAGE_LENGTH:
                error_msg = "Invalid message length";
                break;
            case VISCA_ERROR_SYNTAX:
                error_msg = "Syntax error";
                break;
            case VISCA_ERROR_CMD_BUFFER_FULL:
                error_msg = "Command buffer full";
                break;
            case VISCA_ERROR_CMD_CANCELLED:
                error_msg = "Command cancelled";
                break;
            case VISCA_ERROR_NO_SOCKET:
                error_msg = "No socket available";
                break;
            case VISCA_ERROR_CMD_NOT_EXECUTABLE:
                error_msg = "Command not executable";
                break;
            default:
                error_msg = "Unknown VISCA error: " + std::to_string(result);
                break;
        }

        return Result<void>::error(error_msg);
    }

    Result<uint16_t> SonyCamera::sendInquiry(const std::function<uint32_t(uint16_t*)>& inquiry) {
        uint16_t value = 0;
        const uint32_t result = inquiry(&value);

        if (result == VISCA_SUCCESS) {
            return Result<uint16_t>::success(value);
        }

        std::string error_msg;
        switch (result) {
            case VISCA_ERROR_MESSAGE_LENGTH:
                error_msg = "Invalid message length";
                break;
            case VISCA_ERROR_SYNTAX:
                error_msg = "Syntax error";
                break;
            case VISCA_ERROR_CMD_BUFFER_FULL:
                error_msg = "Command buffer full";
                break;
            case VISCA_ERROR_CMD_CANCELLED:
                error_msg = "Command cancelled";
                break;
            case VISCA_ERROR_NO_SOCKET:
                error_msg = "No socket available";
                break;
            case VISCA_ERROR_CMD_NOT_EXECUTABLE:
                error_msg = "Command not executable";
                break;
            default:
                error_msg = "Unknown VISCA error: " + std::to_string(result);
                break;
        }

        return Result<uint16_t>::error(error_msg);
    }
}
