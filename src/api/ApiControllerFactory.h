#pragma once

#include <memory>

namespace camera_service::common {
    struct ApiConfig;
}

namespace camera_service::core {
    class ICore;
}

namespace camera_service::api {
    class ApiController;

    class ApiControllerFactory {
    public:
        static std::unique_ptr<ApiController> createController(
            std::unique_ptr<core::ICore> core, const common::ApiConfig& config);
    };
}

