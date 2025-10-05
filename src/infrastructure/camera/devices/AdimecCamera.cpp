#include "AdimecCamera.h"

#include <thread>
#include <chrono>

#include "common/Logger/Logger.h"

#define NFOV_CAMERA_LOCK_TIMEOUT_MS 200 //TODO: remove when real async camera control is implemented

namespace camera_service::infrastructure {
    AdimecCamera::AdimecCamera(std::unique_ptr<RegistersMapManager> fpga_manager) : fpga_(std::move(fpga_manager)) {
    }

    Result<void> AdimecCamera::setZoom(const types::zoom zoom) const {
        const auto result = fpga_->setValue(REG::ZOOM, static_cast<uint32_t>(zoom));
        if (result.isError()) {
            return Result<void>::error("Failed to set zoom: " + result.error());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::zoom> AdimecCamera::getZoom() const {
        const auto zoom_result = fpga_->getValue(REG::ZOOM);
        if (zoom_result.isError()) {
            return Result<types::zoom>::error("Failed to get zoom: " + zoom_result.error());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::zoom>::success(static_cast<types::zoom>(zoom_result.value()));
    }

    Result<void> AdimecCamera::setFocus(const types::focus focus) const {
        const auto result = fpga_->setValue(REG::FOCUS, static_cast<uint32_t>(focus));
        if (result.isError()) {
            return Result<void>::error("Failed to set focus: " + result.error());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<types::focus> AdimecCamera::getFocus() const {
        const auto focus_result = fpga_->getValue(REG::FOCUS);
        if (focus_result.isError()) {
            return Result<types::focus>::error("Failed to get focus: " + focus_result.error());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::focus>::success(static_cast<types::focus>(focus_result.value()));
    }

    Result<types::info> AdimecCamera::getInfo() const {
        const auto info_result = fpga_->getValue(REG::VERSION);
        if (info_result.isError()) {
            return Result<types::info>::error("Failed to get camera info: " + info_result.error());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::info>::success(std::to_string(info_result.value()));
    }

    Result<void> AdimecCamera::connect() {
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<void> AdimecCamera::disconnect() {
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    types::ZoomRange AdimecCamera::getZoomLimits() const {
        return zoom_limits_;
    }

    types::FocusRange AdimecCamera::getFocusLimits() const {
        return focus_limits_;
    }
}
