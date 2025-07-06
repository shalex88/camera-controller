#include "NfovCamera.h"

namespace camera_service::data {
    NfovCamera::~NfovCamera() {
        disconnect();
    }

    Result<void> NfovCamera::setZoom(const types::zoom zoom) {
        //TODO: implement real value setting
        current_zoom_ = zoom;
        return Result<void>::success();
    }

    Result<types::zoom> NfovCamera::getZoom() const {
        //TODO: implement real value retrieving
        return Result<types::zoom>::success(current_zoom_);
    }

    Result<void> NfovCamera::setFocus(const types::focus value) {
        //TODO: implement real value setting
        current_zoom_ = value;
        return Result<void>::success();
    }

    Result<types::focus> NfovCamera::getFocus() const {
        //TODO: implement real value retrieving
        return Result<types::focus>::success(current_zoom_);
    }

    Result<void> NfovCamera::connect() {
        return Result<void>::success();
    }

    Result<void> NfovCamera::disconnect() {
        return Result<void>::success();
    }

    types::CameraLimits NfovCamera::getLimits() const {
        return limits_;
    }
}
