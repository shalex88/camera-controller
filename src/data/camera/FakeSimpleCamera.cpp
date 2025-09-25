#include "FakeSimpleCamera.h"

namespace camera_service::data {
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

    Result<void> FakeSimpleCamera::connect() {
        return Result<void>::success();
    }

    Result<void> FakeSimpleCamera::disconnect() {
        return Result<void>::success();
    }

    types::ZoomRange FakeSimpleCamera::getZoomLimits() const {
        return zoom_limits_;
    }
}
