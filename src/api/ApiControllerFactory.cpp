#include "ApiControllerFactory.h"
#include "api/RequestHandler.h"
#include "api/GrpcTransport.h"
#include "api/ApiController.h"

namespace camera_service::api {
    std::unique_ptr<ApiController> ApiControllerFactory::createController(
        std::unique_ptr<core::ICore> core, std::shared_ptr<LayerLogger> logger, const ApiConfig& config) {
        if (config.api == "grpc") {
            auto request_handler = std::make_shared<RequestHandler>(std::move(core), logger);
            auto transport = std::make_unique<GrpcTransport>(request_handler, logger);
            return std::make_unique<ApiController>(request_handler, std::move(transport), config.server_address, logger);
        }
        throw std::invalid_argument("Unknown API controller type");
    }
}
