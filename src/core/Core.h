#pragma once
#include <memory>

#include "core/ICore.h"
#include "common/types/CameraTypes.h"

namespace camera_service::data {
    class ICamera;
}

namespace camera_service::core {
    class Core final : public ICore {
    public:
        explicit Core(std::unique_ptr<data::ICamera> camera);
        ~Core() override;

        bool initialize() override;
        void shutdown() override;

        void setZoom(types::zoom zoom_level) override;
        types::zoom getZoom() const override;

        void setFocus(types::focus focus_value) override;
        types::focus getFocus() const override;

    private:
        std::unique_ptr<data::ICamera> camera_;
        bool initialized_;
    };
}
