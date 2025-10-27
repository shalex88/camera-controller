#include "ApiControllerFactory.h"

#include "api/ApiController.h"
#include "api/GrpcTransport.h"
#include "api/RequestHandler.h"
#include "common/config/ConfigManager.h"
#include "core/ICore.h"

namespace camera_service::api {
    std::unique_ptr<ApiController> ApiControllerFactory::createController(
        std::unique_ptr<core::ICore> core, std::shared_ptr<common::LayerLogger> logger, const common::ApiConfig& config) {
        if (!core) {
            throw std::invalid_argument("Core cannot be null");
        }

        if (!logger) {
            throw std::invalid_argument("Logger cannot be null");
        }

        if (config.api == "grpc") {
            auto request_handler = std::make_shared<RequestHandler>(std::move(core), logger);
            auto transport = std::make_unique<GrpcTransport>(request_handler, logger);
            return std::make_unique<ApiController>(request_handler, std::move(transport), config.server_address, logger);
        }

        throw std::invalid_argument("Unknown API controller type: " + config.api);
    }
}
