#pragma once
#include <memory>

#include "core/ICore.h"

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

        void setZoom(double zoom_level) override;
        double getZoom() const override;

        void setFocus(double focus_value) override;
        double getFocus() const override;

    private:
        std::unique_ptr<data::ICamera> camera_;
        bool initialized_;
    };
}
