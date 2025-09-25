#pragma once
#include <memory>

#include "core/ICore.h"
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"
#include "common/Logger/Logger.h"

namespace camera_service::data {
    class ICameraHal;
}

namespace camera_service::core {
    class Core final : public ICore {
    public:
        explicit Core(std::unique_ptr<data::ICameraHal> camera,
                     std::shared_ptr<LayerLogger> logger);
        ~Core() override;

        // ICameraService implementation
        Result<void> initialize() override;
        Result<void> shutdown() override;

        // Business methods for zoom operations
        Result<void> setZoom(types::zoom zoom_level) const override;
        Result<types::zoom> getZoom() const override;
        Result<void> goToMinZoom() const override;
        Result<void> goToMaxZoom() const override;

        // Business methods for focus operations
        Result<void> setFocus(types::focus focus_value) const override;
        Result<types::focus> getFocus() const override;
        Result<void> enableAutoFocus(bool on) const override;
        Result<bool> isAutoFocusEnabled() const override;

        // Business methods for info operations
        Result<types::info> getInfo() const override;

    protected:
        // No longer needed - Core delegates directly to HAL business methods

    private:
        bool isInitialized() const;
        std::unique_ptr<data::ICameraHal> camera_;
        std::shared_ptr<LayerLogger> logger_;
        bool is_initialized_;
    };
}
