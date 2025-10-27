#pragma once

#include <memory>

namespace camera_service::common {
    struct DataConfig;
    class LayerLogger;
}

namespace camera_service::infrastructure {
    class ICamera;

    class CameraFactory {
    public:
        static std::unique_ptr<ICamera> createCamera(
            std::shared_ptr<common::LayerLogger> logger, const common::DataConfig& config);
    };
}
