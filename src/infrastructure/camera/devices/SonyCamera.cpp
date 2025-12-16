#include "SonyCamera.h"

#include <utility>

#include "infrastructure/camera/protocol/visca/ViscaProtocol.h"

namespace service::infrastructure {
    SonyCamera::SonyCamera(std::unique_ptr<ViscaProtocol> protocol)
        : protocol_(std::move(protocol)) {
    }

    SonyCamera::~SonyCamera() {
        [[maybe_unused]] const auto disconnect_result = close();
    }

    Result<void> SonyCamera::setZoom(const types::zoom zoom) const {
        return protocol_->setZoomValue(static_cast<uint16_t>(zoom));
    }

    Result<types::zoom> SonyCamera::getZoom() const {
        const auto result = protocol_->getZoomValue();
        if (result.isError()) {
            return Result<types::zoom>::error(result.error());
        }

        return Result<types::zoom>::success(static_cast<types::zoom>(result.value()));
    }

    types::ZoomRange SonyCamera::getZoomLimits() const {
        return zoom_limits_;
    }

    Result<void> SonyCamera::setFocus(const types::focus focus) const {
        if (const auto auto_focus_result = isAutoFocusEnabled(); auto_focus_result.isError()) {
            return Result<void>::error("Failed to get focus mode: " + auto_focus_result.error());
        } else if (auto_focus_result.isSuccess() && auto_focus_result.value()) {
            return Result<void>::error("Cannot set focus value while autofocus is enabled");
        }

        return protocol_->setFocusValue(static_cast<uint16_t>(focus));
    }

    Result<types::focus> SonyCamera::getFocus() const {
        if (const auto auto_focus_result = isAutoFocusEnabled(); auto_focus_result.isError()) {
            return Result<types::focus>::error("Failed to get focus mode: " + auto_focus_result.error());
        } else if (auto_focus_result.isSuccess() && auto_focus_result.value()) {
            return Result<types::focus>::error("Cannot get focus value while autofocus is enabled");
        }

        const auto result = protocol_->getFocusValue();
        if (result.isError()) {
            return Result<types::focus>::error(result.error());
        }

        return Result<types::focus>::success(static_cast<types::focus>(result.value()));
    }

    types::FocusRange SonyCamera::getFocusLimits() const {
        return focus_limits_;
    }

    Result<void> SonyCamera::enableAutoFocus(const bool on) const {
        return protocol_->setFocusAuto(on);
    }

    Result<bool> SonyCamera::isAutoFocusEnabled() const {
        return protocol_->getFocusAuto();
    }

    Result<types::info> SonyCamera::getInfo() const {
        const auto result = protocol_->getCameraInfo();
        if (result.isError()) {
            return Result<types::info>::error(result.error());
        }

        return Result<types::info>::success(static_cast<types::info>(result.value()));
    }

    Result<void> SonyCamera::stabilize(const bool on) const {
        return protocol_->setCamStabilizer(on);
    }

    Result<void> SonyCamera::open() {
        if (const auto result = protocol_->open(); result.isError()) {
            return Result<void>::error(result.error());
        }

        if (const auto result = protocol_->setAddress(); result.isError()) {
            return Result<void>::error(result.error());
        }

        if (const auto result = protocol_->clear(); result.isError()) {
            return Result<void>::error(result.error());
        }

        return Result<void>::success();
    }

    Result<void> SonyCamera::close() {
        return protocol_->close();
    }
}
