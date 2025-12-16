#include "FakeSimpleCamera.h"

namespace service::infrastructure {
    Result<void> FakeSimpleCamera::setZoom(const types::zoom zoom) const {
        zoom_ = zoom;
        return Result<void>::success();
    }

    Result<types::zoom> FakeSimpleCamera::getZoom() const {
        return Result<types::zoom>::success(zoom_);
    }

    Result<types::info> FakeSimpleCamera::getInfo() const {
        return Result<types::info>::success(info_);
    }

    Result<void> FakeSimpleCamera::open() {
        return Result<void>::success();
    }

    Result<void> FakeSimpleCamera::close() {
        return Result<void>::success();
    }

    types::ZoomRange FakeSimpleCamera::getZoomLimits() const {
        return zoom_limits_;
    }

    Result<void> FakeSimpleCamera::setFocus(const types::focus focus) const {
        focus_ = focus;
        return Result<void>::success();
    }

    Result<types::focus> FakeSimpleCamera::getFocus() const {
        return Result<types::focus>::success(focus_);
    }

    types::FocusRange FakeSimpleCamera::getFocusLimits() const {
        return focus_limits_;
    }
}
