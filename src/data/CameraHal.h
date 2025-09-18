#pragma once
#include "ICameraHal.h"

#include <memory>

#include "ICameraHw.h"
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"
#include "common/Logger/Logger.h"

namespace camera_service::data {
    class CameraHal final : public ICameraHal {
    public:
        explicit CameraHal(std::unique_ptr<ICameraHw> camera_strategy,
                          std::shared_ptr<LayerLogger> logger);
        ~CameraHal() override;

        Result<void> setZoom(types::zoom normalized_zoom) const override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus normalized_focus) const override;
        Result<types::focus> getFocus() const override;
        Result<types::info> getInfo() const override;
        Result<void> setMinZoom() const override;
        Result<void> setMaxZoom() const override;
        Result<void> connect() override;
        Result<void> disconnect() override;
        bool isConnected() const override;

    private:
        std::unique_ptr<ICameraHw> camera_hw_;
        std::shared_ptr<LayerLogger> logger_;
        types::CameraLimits limits_;
        bool connected_ {false};

        static bool isValidNormalizedZoom(types::zoom value);
        static bool isValidNormalizedFocus(types::focus value);
        bool isValidCameraZoom(types::zoom value) const;
        bool isValidCameraFocus(types::focus value) const;
        types::zoom normalizeZoom(types::zoom camera_zoom) const;
        types::focus normalizeFocus(types::focus camera_focus) const;
        types::zoom denormalizeZoom(types::zoom normalized_zoom) const;
        types::focus denormalizeFocus(types::focus normalized_focus) const;
    };
}