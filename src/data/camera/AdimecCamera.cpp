#include "AdimecCamera.h"

#include <thread>
#include <chrono>

#include "common/Logger/Logger.h"

#define NFOV_CAMERA_LOCK_TIMEOUT_MS 200

namespace camera_service::data {
    AdimecCamera::AdimecCamera(std::unique_ptr<RegistersMapManager> fpga_manager):
        fpga_(std::move(fpga_manager)) {
    }

    Result<void> AdimecCamera::setZoom(const types::zoom zoom) const {
        fpga_->setValue(REG::ZOOM, static_cast<uint32_t>(zoom));
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::zoom> AdimecCamera::getZoom() const {
        const auto zoom = fpga_->getValue(REG::ZOOM);
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::zoom>::success(static_cast<types::zoom>(zoom));
    }

    Result<void> AdimecCamera::setFocus(const types::focus focus) const {
        fpga_->setValue(REG::FOCUS, static_cast<uint32_t>(focus));
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::focus> AdimecCamera::getFocus() const {
        const auto focus = fpga_->getValue(REG::FOCUS);
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::focus>::success(static_cast<types::focus>(focus));
    }

    Result<types::info> AdimecCamera::getInfo() const {
        const auto info = fpga_->getValue(REG::VERSION);
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::info>::success(std::to_string(info));
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
