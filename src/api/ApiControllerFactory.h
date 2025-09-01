#pragma once
#include <memory>

#include "core/ICore.h"
#include "api/ApiController.h"
#include "common/Config/ConfigManager.h"
#include "common/Logger/Logger.h"

namespace camera_service::api {
    class ApiControllerFactory {
    public:
        static std::unique_ptr<ApiController> createController(
            std::unique_ptr<core::ICore> core,
            std::shared_ptr<LayerLogger> logger, const ApiConfig& config);
    };
}
