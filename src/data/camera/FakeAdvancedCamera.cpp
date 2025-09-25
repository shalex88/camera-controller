#include "FakeAdvancedCamera.h"

namespace camera_service::data {
    Result<void> FakeAdvancedCamera::setZoom(const types::zoom zoom) const {
        zoom_ = zoom;
        return Result<void>::success();
    }

    Result<types::zoom> FakeAdvancedCamera::getZoom() const {
        return Result<types::zoom>::success(zoom_);
    }

    Result<void> FakeAdvancedCamera::setFocus(const types::focus focus) const {
        focus_ = focus;
        return Result<void>::success();
    }

    Result<types::focus> FakeAdvancedCamera::getFocus() const {
        return Result<types::focus>::success(focus_);
    }

    Result<types::info> FakeAdvancedCamera::getInfo() const {
        return Result<types::info>::success(info_);
    }

    Result<void> FakeAdvancedCamera::connect() {
        return Result<void>::success();
    }

    Result<void> FakeAdvancedCamera::disconnect() {
        return Result<void>::success();
    }

    types::ZoomRange FakeAdvancedCamera::getZoomLimits() const {
        return zoom_limits_;
    }

    types::FocusRange FakeAdvancedCamera::getFocusLimits() const {
        return focus_limits_;
    }
}
