#include "FakeAdvancedCamera.h"

namespace service::infrastructure {
    namespace {
        constexpr common::types::ZoomRange zoom_limits_{
            .min = 0x0,
            .max = 0xFF
        };

        constexpr common::types::FocusRange focus_limits_{
            .min = 0x0,
            .max = 0xFF
        };
    } // unnamed namespace

    Result<void> FakeAdvancedCamera::setZoom(const common::types::zoom zoom) const {
        std::lock_guard lock(state_mutex_);
        zoom_ = zoom;
        return Result<void>::success();
    }

    Result<common::types::zoom> FakeAdvancedCamera::getZoom() const {
        std::lock_guard lock(state_mutex_);
        return Result<common::types::zoom>::success(zoom_);
    }

    Result<void> FakeAdvancedCamera::setFocus(common::types::focus focus) const {
        std::lock_guard lock(state_mutex_);
        if (auto_focus_enabled_) {
            return Result<void>::error("Cannot set focus value while autofocus is enabled");
        }
        focus_ = focus;
        return Result<void>::success();
    }

    Result<common::types::focus> FakeAdvancedCamera::getFocus() const {
        std::lock_guard lock(state_mutex_);
        if (auto_focus_enabled_) {
            return Result<common::types::focus>::error("Cannot get focus value while autofocus is enabled");
        }
        return Result<common::types::focus>::success(focus_);
    }

    Result<common::types::info> FakeAdvancedCamera::getInfo() const {
        std::lock_guard lock(state_mutex_);
        return Result<common::types::info>::success(info_);
    }

    Result<void> FakeAdvancedCamera::open() {
        return Result<void>::success();
    }

    Result<void> FakeAdvancedCamera::close() {
        return Result<void>::success();
    }

    Result<void> FakeAdvancedCamera::enableAutoFocus(bool enable) const {
        std::lock_guard lock(state_mutex_);
        auto_focus_enabled_ = enable;
        return Result<void>::success();
    }

    Result<bool> FakeAdvancedCamera::isAutoFocusEnabled() const {
        std::lock_guard lock(state_mutex_);
        return Result<bool>::success(auto_focus_enabled_);
    }

    Result<void> FakeAdvancedCamera::stabilize(bool enable) const {
        std::lock_guard lock(state_mutex_);
        stabilize_enabled_ = enable;
        return Result<void>::success();
    }

    Result<bool> FakeAdvancedCamera::isStabilizationEnabled() const {
        std::lock_guard lock(state_mutex_);
        return Result<bool>::success(stabilize_enabled_);
    }

    common::types::ZoomRange FakeAdvancedCamera::getZoomLimits() const {
        return zoom_limits_;
    }

    common::types::FocusRange FakeAdvancedCamera::getFocusLimits() const {
        return focus_limits_;
    }
} // namespace service::infrastructure
