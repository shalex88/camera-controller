#include "NfovCameraHw.h"

#include "common/Logger/Logger.h"

namespace camera_service::data {
    NfovCameraHw::~NfovCameraHw() {
        if (disconnect().isError()) {
            LOG_ERROR("Failed to disconnect NFOV camera");
        }
    }

    Result<void> NfovCameraHw::setZoom(const types::zoom zoom) {
        //TODO: implement real value setting
        current_zoom_ = zoom;
        return Result<void>::success();
    }

    Result<types::zoom> NfovCameraHw::getZoom() const {
        //TODO: implement real value retrieving
        return Result<types::zoom>::success(current_zoom_);
    }

    Result<void> NfovCameraHw::setFocus(const types::focus value) {
        //TODO: implement real value setting
        current_zoom_ = value;
        return Result<void>::success();
    }

    Result<types::focus> NfovCameraHw::getFocus() const {
        //TODO: implement real value retrieving
        return Result<types::focus>::success(current_zoom_);
    }

    Result<void> NfovCameraHw::connect() {
        return Result<void>::success();
    }

    Result<void> NfovCameraHw::disconnect() {
        return Result<void>::success();
    }

    types::CameraLimits NfovCameraHw::getLimits() const {
        return limits_;
    }
}
