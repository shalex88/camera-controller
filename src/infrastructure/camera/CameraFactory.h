#pragma once

#include <memory>

namespace camera_service::common {
    struct InfrastructureConfig;
}

namespace camera_service::infrastructure {
    class ICamera;

    class CameraFactory {
    public:
        static std::unique_ptr<ICamera> createCamera(const common::InfrastructureConfig& config);
    };
}
