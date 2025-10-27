#pragma once
#include <memory>

namespace camera_service::common {
    struct CoreConfig;
}

namespace camera_service::infrastructure {
    class ICamera;
}

namespace camera_service::core {
    class ICore;

    class CoreFactory {
    public:
        static std::unique_ptr<ICore> createCore(
            std::unique_ptr<infrastructure::ICamera> camera, const common::CoreConfig& config);
    };
}
