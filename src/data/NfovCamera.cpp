#include "NfovCamera.h"

namespace camera_service::data {
    NfovCamera::NfovCamera()
        : zoom_level_(1.0)
          , focus_value_(0.0)
          , connected_(false) {
    }

    NfovCamera::~NfovCamera() {
        if (connected_) {
            disconnect();
        }
    }

    Result<void> NfovCamera::setZoom(const types::zoom zoom_level) {
        zoom_level_ = zoom_level;
        return Result<void>::success();
    }

    Result<types::zoom> NfovCamera::getZoom() const {
        return Result<types::zoom>::success(zoom_level_);
    }

    Result<void> NfovCamera::setFocus(const types::focus focus_value) {
        focus_value_ = focus_value;
        return Result<void>::success();
    }

    Result<types::focus> NfovCamera::getFocus() const {
        return Result<types::focus>::success(focus_value_);
    }

    Result<void> NfovCamera::connect() {
        connected_ = true;
        return Result<void>::success();
    }

    Result<void> NfovCamera::disconnect() {
        connected_ = false;
        return Result<void>::success();
    }

    bool NfovCamera::isConnected() const {
        return connected_;
    }
}