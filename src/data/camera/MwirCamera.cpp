#include "MwirCamera.h"

#include "data/transport/ethernet/MwirOpcodes.h"

namespace camera_service::data {
    MwirCamera::MwirCamera(std::unique_ptr<ItlProtocol> protocol) : protocol_(std::move(protocol)) {
        if (!protocol_) {
            throw std::invalid_argument("Protocol cannot be null");
        }
    }

    Result<void> MwirCamera::setZoom(const types::zoom zoom) const {
        zoom_ = zoom;
        return Result<void>::success();
    }

    Result<types::zoom> MwirCamera::getZoom() const {
        return Result<types::zoom>::success(zoom_);
    }

    Result<void> MwirCamera::setFocus(const types::focus focus) const {
        if (auto_focus_enabled_) {
            return Result<void>::error("Cannot set focus value while auto focus is enabled");
        }
        focus_ = focus;
        return Result<void>::success();
    }

    Result<types::focus> MwirCamera::getFocus() const {
        if (auto_focus_enabled_) {
            return Result<types::focus>::error("Cannot get focus value while auto focus is enabled");
        }
        return Result<types::focus>::success(focus_);
    }

    Result<types::info> MwirCamera::getInfo() const {
        constexpr std::vector<uint8_t> payload;

        const auto info = protocol_->sendPayload(MWIR_GET_VERSION, payload);
        if (info.isError()) {
            return Result<types::info>::error(info.error());
        }
        const std::string result = "v" + std::to_string(info.value().at(0)) + "." +
                                  std::to_string(info.value().at(1)) + "." +
                                  std::to_string(info.value().at(2)) + "." +
                                  std::to_string(info.value().at(3));
        return Result<types::info>::success(result);
    }

    Result<void> MwirCamera::connect() {
        if (const auto result = protocol_->connect(); result.isError()) {
            return Result<void>::error(result.error());
        }
        return Result<void>::success();
    }

    Result<void> MwirCamera::disconnect() {
        if (const auto result = protocol_->disconnect(); !result.isError()) {
            return Result<void>::success();
        }
        return Result<void>::error("Failed to disconnect");
    }

    Result<void> MwirCamera::enableAutoFocus(const bool on) const {
        auto_focus_enabled_ = on;
        return Result<void>::success();
    }

    Result<bool> MwirCamera::isAutoFocusEnabled() const {
        return Result<bool>::success(auto_focus_enabled_);
    }

    types::ZoomRange MwirCamera::getZoomLimits() const {
        return zoom_limits_;
    }

    types::FocusRange MwirCamera::getFocusLimits() const {
        return focus_limits_;
    }
}
