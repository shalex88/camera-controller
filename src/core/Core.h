#pragma once
#include <memory>

#include "core/ICore.h"
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"
#include "common/Logger/Logger.h"
#include "infrastructure/camera/hal/ICameraHal.h"

namespace camera_service::core {
    class Core final : public ICore {
    public:
        explicit Core(std::unique_ptr<infrastructure::ICameraHal> camera,
                     std::shared_ptr<LayerLogger> logger);
        ~Core() override;

        // ICore implementation
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

        // Business methods for info operations
        Result<types::info> getInfo() const override;

        // Business methods for advanced operations
        Result<void> stabilize(bool on) const override;

    private:
        bool isInitialized() const;
        std::unique_ptr<infrastructure::ICameraHal> camera_;
        std::shared_ptr<LayerLogger> logger_;
        bool is_initialized_;
    };
}
