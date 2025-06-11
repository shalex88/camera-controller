#pragma once
#include <memory>

#include "core/ICore.h"

namespace camera_service::data {
    class ICamera;
}

namespace camera_service::core {
    class CameraCore final : public ICore {
    public:
        explicit CameraCore(std::unique_ptr<data::ICamera> camera);
        ~CameraCore() override;

        bool initialize() override;
        void shutdown() override;

        void setZoom(double zoomLevel) override;
        double getZoom() const override;

        void setFocus(double focusValue) override;
        double getFocus() const override;

    private:
        std::unique_ptr<data::ICamera> m_camera;
        bool m_initialized;
    };
}
