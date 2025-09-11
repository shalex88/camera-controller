#include "SonyCameraVisca.h"

#include <thread>
#include <chrono>
#include <cstring>
#include <utility>

#include "common/Logger/Logger.h"

namespace camera_service::data {
    SonyCameraVisca::SonyCameraVisca(std::string device_path)
        : device_path_(std::move(device_path)) {
    }

    SonyCameraVisca::~SonyCameraVisca() {
        if (is_connected_) {
            if (disconnect().isError()) {
                LOG_ERROR("Failed to disconnect camera in destructor");
            }
        }
    }

    Result<void> SonyCameraVisca::setZoom(const types::zoom zoom) {
        if (!is_connected_) {
            return Result<void>::error("Camera not connected");
        }

        const uint16_t visca_zoom = convertZoomToVisca(zoom);

        return sendCommand([this, visca_zoom]() {
            return VISCA_set_zoom_value(&interface_, &camera_, visca_zoom);
        });
    }

    Result<types::zoom> SonyCameraVisca::getZoom() const {
        if (!is_connected_) {
            return Result<types::zoom>::error("Camera not connected");
        }

        auto result = sendInquiry([this](uint16_t* value) {
            return VISCA_get_zoom_value(&interface_, &camera_, value);
        });

        if (result.isError()) {
            return Result<types::zoom>::error("Failed to get zoom value: " + result.error());
        }

        const types::zoom zoom = convertZoomFromVisca(result.value());
        return Result<types::zoom>::success(zoom);
    }

    Result<void> SonyCameraVisca::setFocus(const types::focus focus) {
        if (!is_connected_) {
            return Result<void>::error("Camera not connected");
        }

        // First set focus to manual mode
        auto manualResult = sendCommand([this]() {
            return VISCA_set_focus_auto(&interface_, &camera_, VISCA_OFF);
        });

        if (manualResult.isError()) {
            return Result<void>::error("Failed to set manual focus mode: " + manualResult.error());
        }

        const uint16_t visca_focus = convertFocusToVisca(focus);

        return sendCommand([this, visca_focus]() {
            return VISCA_set_focus_value(&interface_, &camera_, visca_focus);
        });
    }

    Result<types::focus> SonyCameraVisca::getFocus() const {
        if (!is_connected_) {
            return Result<types::focus>::error("Camera not connected");
        }

        auto result = sendInquiry([this](uint16_t* value) {
            return VISCA_get_focus_value(&interface_, &camera_, value);
        });

        if (result.isError()) {
            return Result<types::focus>::error("Failed to get focus value: " + result.error());
        }

        const types::focus focus = convertFocusFromVisca(result.value());
        return Result<types::focus>::success(focus);
    }

    Result<types::info> SonyCameraVisca::getInfo() const {
        if (!is_connected_) {
            return Result<types::info>::error("Camera not connected");
        }

        // Get camera information
        auto infoResult = sendCommand([this]() {
            return VISCA_get_camera_info(&interface_, &camera_);
        });

        if (infoResult.isError()) {
            return Result<types::info>::error("Failed to get camera info: " + infoResult.error());
        }

        // Format camera information as string
        std::string info_str = "Sony VISCA Camera - ";
        info_str += "Vendor: 0x" + std::to_string(camera_.vendor) + ", ";
        info_str += "Model: 0x" + std::to_string(camera_.model) + ", ";
        info_str += "ROM Version: 0x" + std::to_string(camera_.rom_version) + ", ";
        info_str += "Address: " + std::to_string(camera_.address);

        return Result<types::info>::success(info_str);
    }

    Result<void> SonyCameraVisca::connect() {
        if (is_connected_) {
            return Result<void>::success();
        }

        // Open serial connection
        uint32_t result = VISCA_open_serial(&interface_, device_path_.c_str());
        if (result != VISCA_SUCCESS) {
            return Result<void>::error("Failed to open serial connection to device: " + device_path_);
        }

        result = VISCA_set_address(&interface_, &camera_address_);
        if (result != VISCA_SUCCESS) {
            VISCA_close_serial(&interface_);
            return Result<void>::error("Failed to set camera address");
        }

        // Clear any pending commands
        result = VISCA_clear(&interface_, &camera_);
        if (result != VISCA_SUCCESS) {
            VISCA_close_serial(&interface_);
            return Result<void>::error("Failed to clear camera commands");
        }

        // Get camera info to verify connection
        result = VISCA_get_camera_info(&interface_, &camera_);
        if (result != VISCA_SUCCESS) {
            VISCA_close_serial(&interface_);
            return Result<void>::error("Failed to get camera information");
        }

        is_connected_ = true;
        LOG_INFO("Connected to Sony VISCA camera at address " + std::to_string(camera_address_));

        return Result<void>::success();
    }

    Result<void> SonyCameraVisca::disconnect() {
        if (!is_connected_) {
            return Result<void>::success();
        }

        uint32_t result = VISCA_close_serial(&interface_);
        if (result != VISCA_SUCCESS) {
            return Result<void>::error("Failed to close serial connection");
        }

        is_connected_ = false;
        LOG_INFO("Disconnected from Sony VISCA camera");

        return Result<void>::success();
    }

    types::CameraLimits SonyCameraVisca::getLimits() const {
        return limits_;
    }

    Result<void> SonyCameraVisca::sendCommand(const std::function<uint32_t()>& command) {
        uint32_t result = command();

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

    Result<uint16_t> SonyCameraVisca::sendInquiry(const std::function<uint32_t(uint16_t*)>& inquiry) {
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

    uint16_t SonyCameraVisca::convertZoomToVisca(types::zoom zoom) {
        // Convert from application zoom range to VISCA zoom range (0x0000 - 0x4000)
        // Assuming application zoom is 0-100 scale
        return static_cast<uint16_t>((zoom * 0x4000) / 100.0);
    }

    types::zoom SonyCameraVisca::convertZoomFromVisca(uint16_t visca_zoom) {
        // Convert from VISCA zoom range to application zoom range
        return static_cast<types::zoom>((visca_zoom * 100.0) / 0x4000);
    }

    uint16_t SonyCameraVisca::convertFocusToVisca(types::focus focus) {
        // Convert from application focus range to VISCA focus range (0x1000 - 0xF000)
        // Assuming application focus is 0-100 scale
        return static_cast<uint16_t>(0x1000 + ((focus * (0xF000 - 0x1000)) / 100.0));
    }

    types::focus SonyCameraVisca::convertFocusFromVisca(uint16_t visca_focus) {
        // Convert from VISCA focus range to application focus range
        if (visca_focus < 0x1000) visca_focus = 0x1000;
        if (visca_focus > 0xF000) visca_focus = 0xF000;

        return static_cast<types::focus>(((visca_focus - 0x1000) * 100.0) / (0xF000 - 0x1000));
    }
}
