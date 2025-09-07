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

        Result<void> setZoom(types::zoom zoom) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus) override;
        Result<types::focus> getFocus() const override;
        Result<void> connect() override;
        Result<void> disconnect() override;
        bool isConnected() const override;

    private:
        std::unique_ptr<ICameraHw> camera_hw_;
        std::shared_ptr<LayerLogger> logger_;
        types::CameraLimits limits_;
        bool connected_ {false};

        bool isValidZoom(types::zoom value) const;
        bool isValidFocus(types::focus value) const;
    };
}