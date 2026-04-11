#include "AdimecCamera.h"

#include "common/logger/Logger.h"
#include "infrastructure/camera/protocol/genicam/GenicamProtocol.h"
#include "infrastructure/camera/protocol/itl/ItlProtocol.h"

namespace service::infrastructure {
    AdimecCamera::AdimecCamera(std::unique_ptr<GenicamProtocol> camera_protocol, std::unique_ptr<ItlProtocol> lens_protocol) :
        camera_protocol_(std::move(camera_protocol)), lens_protocol_(std::move(lens_protocol)) {
        if (!camera_protocol_) {
            throw std::invalid_argument("Camera protocol cannot be null");
        }
        if (!lens_protocol_) {
            throw std::invalid_argument("Lens protocol cannot be null");
        }
    }

    Result<void> AdimecCamera::open() {
        if (const auto camera_result = camera_protocol_->open(); camera_result.isError()) {
            return Result<void>::error("Failed to connect to Adimec camera: " + camera_result.error());
        }

        if (const auto lens_result = lens_protocol_->open(); lens_result.isError()) {
            if (const auto rollback_result = camera_protocol_->close(); rollback_result.isError()) {
                LOG_ERROR("Failed to roll back Adimec camera connection after lens failure: {}", rollback_result.error());
            }

            return Result<void>::error("Failed to connect to Adimec lens: " + lens_result.error());
        }

        return Result<void>::success();
    }

    Result<void> AdimecCamera::close() {
        std::string error_message;

        if (const auto lens_result = lens_protocol_->close(); lens_result.isError()) {
            error_message = "Failed to disconnect from Adimec lens: " + lens_result.error();
        }

        if (const auto camera_result = camera_protocol_->close(); camera_result.isError()) {
            if (!error_message.empty()) {
                error_message.append("; ");
            }
            error_message.append("Failed to disconnect from Adimec camera: ").append(camera_result.error());
        }

        if (!error_message.empty()) {
            return Result<void>::error(error_message);
        }

        return Result<void>::success();
    }

    Result<common::types::info> AdimecCamera::getInfo() const {
        std::string info;

        if (const auto vendor = camera_protocol_->getDeviceVendorName(); vendor.isSuccess()) {
            info += "Vendor: " + vendor.value() + "";
        }

        if (const auto model = camera_protocol_->getDeviceModelName(); model.isSuccess()) {
            info += "Model: " + model.value() + "";
        }

        if (const auto manufacturer_info = camera_protocol_->getDeviceManufacturerInfo(); manufacturer_info.isSuccess()) {
            info += "Manufacturer Info: " + manufacturer_info.value() + " ";
        }

        if (const auto firmware = camera_protocol_->getDeviceFirmwareVersion(); firmware.isSuccess()) {
            info += "Firmware Version: " + firmware.value();
        }

        if (info.empty()) {
            return Result<common::types::info>::error("Failed to retrieve camera information");
        }

        return Result<common::types::info>::success(info);
    }
} // namespace service::infrastructure
