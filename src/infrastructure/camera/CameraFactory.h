#pragma once

#include <memory>

namespace camera_service::common {
    struct DataConfig;
}

namespace camera_service::infrastructure {
    class ICamera;

    class CameraFactory {
    public:
        static std::unique_ptr<ICamera> createCamera(const common::DataConfig& config);
    };
}
