#include "FakeCamera.h"

namespace camera_service::data {
    Result<void> FakeCamera::setZoom(const types::zoom zoom) const {
        zoom_ = zoom;
        return Result<void>::success();
    }

    Result<types::zoom> FakeCamera::getZoom() const {
        return Result<types::zoom>::success(zoom_);
    }

    Result<void> FakeCamera::setFocus(const types::focus focus) const {
        focus_ = focus;
        return Result<void>::success();
    }

    Result<types::focus> FakeCamera::getFocus() const {
        return Result<types::focus>::success(focus_);
    }

    Result<types::info> FakeCamera::getInfo() const {
        return Result<types::info>::success(info_);
    }

    Result<void> FakeCamera::connect() {
        return Result<void>::success();
    }

    Result<void> FakeCamera::disconnect() {
        return Result<void>::success();
    }

    types::CameraLimits FakeCamera::getLimits() const {
        return limits_;
    }

    Result<void> FakeCamera::setMinZoom() const {
        return setZoom(limits_.min_zoom);
    }

    Result<void> FakeCamera::setMaxZoom() const {
        return setZoom(limits_.max_zoom);
    }
}
