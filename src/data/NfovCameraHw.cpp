#include "NfovCameraHw.h"

#include <thread>
#include <chrono>

#include "common/Logger/Logger.h"

#define NFOV_CAMERA_LOCK_TIMEOUT_MS 200

namespace camera_service::data {
    NfovCameraHw::~NfovCameraHw() {
        if (disconnect().isError()) {
            LOG_ERROR("Failed to disconnect NFOV camera");
        }
    }

    Result<void> NfovCameraHw::setZoom(const types::zoom zoom) {
        std::lock_guard lock(mutex_);

        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        // fpga.setValue(REG::SET_ZOOM, static_cast<uint32_t>(zoom));
        current_zoom_ = zoom;
        return Result<void>::success();
    }

    Result<types::zoom> NfovCameraHw::getZoom() const {
        std::lock_guard lock(mutex_);

        // const auto zoom = fpga.getValue(REG::GET_ZOOM);
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::zoom>::success(current_zoom_);
    }

    Result<void> NfovCameraHw::setFocus(const types::focus focus) {
        std::lock_guard lock(mutex_);

        // fpga.setValue(REG::SET_FOCUS, static_cast<uint32_t>(focus));
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        current_focus_ = focus;
        return Result<void>::success();
    }

    Result<types::focus> NfovCameraHw::getFocus() const {
        std::lock_guard lock(mutex_);

        // const auto focus = fpga.getValue(REG::GET_FOCUS);
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<types::focus>::success(current_focus_);
    }

    Result<void> NfovCameraHw::connect() {
        std::lock_guard lock(mutex_);
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    Result<void> NfovCameraHw::disconnect() {
        std::lock_guard lock(mutex_);
        std::this_thread::sleep_for(std::chrono::milliseconds(NFOV_CAMERA_LOCK_TIMEOUT_MS));
        return Result<void>::success();
    }

    types::CameraLimits NfovCameraHw::getLimits() const {
        return limits_;
    }
}
