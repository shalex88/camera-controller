#include "NfovCameraHw.h"

#include "common/Logger/Logger.h"

namespace camera_service::data {
    NfovCameraHw::~NfovCameraHw() {
        if (disconnect().isError()) {
            LOG_ERROR("Failed to disconnect NFOV camera");
        }
    }

    Result<void> NfovCameraHw::setZoom(const types::zoom zoom) {
        std::lock_guard lock(mutex_);
        //TODO: implement real value setting
        current_zoom_ = zoom;
        return Result<void>::success();
    }

    Result<types::zoom> NfovCameraHw::getZoom() const {
        std::lock_guard lock(mutex_);
        //TODO: implement real value retrieving
        return Result<types::zoom>::success(current_zoom_);
    }

    Result<void> NfovCameraHw::setFocus(const types::focus value) {
        std::lock_guard lock(mutex_);
        //TODO: implement real value setting
        current_focus_ = value;
        return Result<void>::success();
    }

    Result<types::focus> NfovCameraHw::getFocus() const {
        std::lock_guard lock(mutex_);
        //TODO: implement real value retrieving
        return Result<types::focus>::success(current_focus_);
    }

    Result<void> NfovCameraHw::connect() {
        std::lock_guard lock(mutex_);
        return Result<void>::success();
    }

    Result<void> NfovCameraHw::disconnect() {
        std::lock_guard lock(mutex_);
        return Result<void>::success();
    }

    types::CameraLimits NfovCameraHw::getLimits() const {
        return limits_;
    }
}
