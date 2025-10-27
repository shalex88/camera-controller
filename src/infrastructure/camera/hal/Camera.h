#pragma once

#include <memory>

#include "common/types/CameraTypes.h"
#include "common/types/Result.h"
#include "infrastructure/camera/hal/ICamera.h"

namespace camera_service::infrastructure {
    class ICameraHw;

    class Camera final : public ICamera {
    public:
        explicit Camera(std::unique_ptr<ICameraHw> camera_strategy);
        ~Camera() override;

        Result<void> setZoom(types::zoom normalized_zoom) const override;
        Result<types::zoom> getZoom() const override;

        Result<void> setFocus(types::focus normalized_focus) const override;
        Result<types::focus> getFocus() const override;
        Result<void> enableAutoFocus(bool on) const override;

        Result<types::info> getInfo() const override;

        Result<void> stabilize(bool on) const override;

        Result<void> connect() override;
        Result<void> disconnect() override;
        bool isConnected() const override;

    private:
        std::unique_ptr<ICameraHw> camera_hw_;
        bool connected_ {false};

        static bool isValidNormalizedZoom(types::zoom value);
        static bool isValidNormalizedFocus(types::focus value);
        bool isValidCameraZoom(types::zoom value) const;
        bool isValidCameraFocus(types::focus value) const;
        types::zoom normalizeZoom(types::zoom camera_zoom) const;
        types::focus normalizeFocus(types::focus camera_focus) const;
        types::zoom denormalizeZoom(types::zoom normalized_zoom) const;
        types::focus denormalizeFocus(types::focus normalized_focus) const;

        types::ZoomRange getZoomLimits() const override;
        types::FocusRange getFocusLimits() const override;

        template <typename Capability>
        Capability* getCapability() const;
    };
}