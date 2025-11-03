#include "AdimecCamera.h"

#include "common/logger/Logger.h"
#include "infrastructure/camera/protocol/genicam/GenicamProtocol.h"
#include "infrastructure/camera/protocol/itl/ItlProtocol.h"

namespace camera_service::infrastructure {
    AdimecCamera::AdimecCamera(std::unique_ptr<GenicamProtocol> camera_protocol, std::unique_ptr<ItlProtocol> lens_protocol) :
        camera_protocol_(std::move(camera_protocol)), lens_protocol_(std::move(lens_protocol)) {
    }

    Result<void> AdimecCamera::setZoom(const types::zoom zoom) const {
        return Result<void>::success();
    }

    Result<types::zoom> AdimecCamera::getZoom() const {
        return Result<types::zoom>::success(static_cast<types::zoom>(0));
    }

    types::ZoomRange AdimecCamera::getZoomLimits() const {
        return zoom_limits_;
    }

    Result<void> AdimecCamera::setFocus(const types::focus focus) const {
        return Result<void>::success();
    }

    Result<types::focus> AdimecCamera::getFocus() const {
        return Result<types::focus>::success(static_cast<types::focus>(0));
    }

    types::FocusRange AdimecCamera::getFocusLimits() const {
        return focus_limits_;
    }

    Result<types::info> AdimecCamera::getInfo() const {
        return Result<types::info>::success(std::to_string(0));
    }

    Result<void> AdimecCamera::open() {
        if (camera_protocol_->open().isError()) {
            return Result<void>::error("Failed to connect to Adimec camera");
        }
        if (lens_protocol_->open().isError()) {
            return Result<void>::error("Failed to connect to Adimec lens");
        }
        return Result<void>::success();
    }

    Result<void> AdimecCamera::close() {
        if (camera_protocol_->close().isError()) {
            return Result<void>::error("Failed to disconnect from Adimec camera");
        }
        if (lens_protocol_->close().isError()) {
            return Result<void>::error("Failed to disconnect from Adimec lens");
        }
        return Result<void>::success();
    }
}
