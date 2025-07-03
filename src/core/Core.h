#pragma once
#include <memory>

#include "core/ICore.h"
#include "common/types/CameraTypes.h"
#include "common/types/Result.h"

namespace camera_service::data {
    class ICamera;
}

namespace camera_service::core {
    class Core final : public ICore {
    public:
        explicit Core(std::unique_ptr<data::ICamera> camera);
        ~Core() override;

        Result<void> initialize() override;
        Result<void> shutdown() override;

        Result<void> setZoom(types::zoom zoom_level) override;
        Result<types::zoom> getZoom() const override;

        Result<void> setFocus(types::focus focus_value) override;
        Result<types::focus> getFocus() const override;

    private:
        bool isInitialized() const;
        std::unique_ptr<data::ICamera> camera_;
        bool is_initialized_;
    };
}
