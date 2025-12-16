#include "FakeAdvancedCamera.h"

namespace service::infrastructure {
    Result<void> FakeAdvancedCamera::setZoom(const types::zoom zoom) const {
        zoom_ = zoom;
        return Result<void>::success();
    }

    Result<types::zoom> FakeAdvancedCamera::getZoom() const {
        return Result<types::zoom>::success(zoom_);
    }

    Result<void> FakeAdvancedCamera::setFocus(const types::focus focus) const {
        if (auto_focus_enabled_) {
            return Result<void>::error("Cannot set focus value while auto focus is enabled");
        }
        focus_ = focus;
        return Result<void>::success();
    }

    Result<types::focus> FakeAdvancedCamera::getFocus() const {
        if (auto_focus_enabled_) {
            return Result<types::focus>::error("Cannot get focus value while auto focus is enabled");
        }
        return Result<types::focus>::success(focus_);
    }

    Result<types::info> FakeAdvancedCamera::getInfo() const {
        return Result<types::info>::success(info_);
    }

    Result<void> FakeAdvancedCamera::open() {
        return Result<void>::success();
    }

    Result<void> FakeAdvancedCamera::close() {
        return Result<void>::success();
    }

    Result<void> FakeAdvancedCamera::enableAutoFocus(const bool on) const {
        auto_focus_enabled_ = on;
        return Result<void>::success();
    }

    Result<bool> FakeAdvancedCamera::isAutoFocusEnabled() const {
        return Result<bool>::success(auto_focus_enabled_);
    }

    Result<void> FakeAdvancedCamera::stabilize(const bool on) const {
        stabilize_enabled_ = on;
        return Result<void>::success();
    }

    types::ZoomRange FakeAdvancedCamera::getZoomLimits() const {
        return zoom_limits_;
    }

    types::FocusRange FakeAdvancedCamera::getFocusLimits() const {
        return focus_limits_;
    }
}
