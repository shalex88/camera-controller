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

        Result<void> initialize() override;
        Result<void> shutdown() override;

        Result<void> setZoom(types::zoom zoom_level) override;
        Result<types::zoom> getZoom() const override;
        Result<void> setFocus(types::focus focus_value) override;
        Result<types::focus> getFocus() const override;

    private:
        bool isInitialized() const;
        std::unique_ptr<data::ICameraHal> camera_;
        std::shared_ptr<LayerLogger> logger_;
        bool is_initialized_;
    };
}
