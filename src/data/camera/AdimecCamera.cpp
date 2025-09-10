#include "AdimecCamera.h"

#include <thread>
#include <chrono>

#include "common/Logger/Logger.h"

#define NFOV_CAMERA_LOCK_TIMEOUT_MS 200

namespace camera_service::data {
    AdimecCamera::~AdimecCamera() {
        if (disconnect().isError()) {
            LOG_ERROR("Failed to disconnect NFOV camera");
        }
    }

    Result<void> AdimecCamera::setZoom(const types::zoom zoom) {
        fpga_->setValue(REG::ZOOM, static_cast<uint32_t>(zoom));
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::zoom> AdimecCamera::getZoom() const {
        const auto zoom = fpga_->getValue(REG::ZOOM);
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::zoom>::success(static_cast<types::zoom>(zoom));
    }

    Result<void> AdimecCamera::setFocus(const types::focus focus) {
        fpga_->setValue(REG::FOCUS, static_cast<uint32_t>(focus));
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::focus> AdimecCamera::getFocus() const {
        const auto focus = fpga_->getValue(REG::FOCUS);
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::focus>::success(static_cast<types::focus>(focus));
    }

    Result<void> AdimecCamera::connect() {
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<void> AdimecCamera::disconnect() {
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    types::CameraLimits AdimecCamera::getLimits() const {
        return limits_;
    }
}
