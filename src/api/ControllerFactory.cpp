#include "ControllerFactory.h"
#include "api/RequestHandler.h"
#include "api/GrpcTransport.h"
#include "api/Controller.h"

namespace camera_service::api {
    std::unique_ptr<Controller> ControllerFactory::createController(
        const std::string& controller_type, const std::string& port, std::unique_ptr<core::ICore> core) {
        if (controller_type == "grpc") {
            auto request_handler = std::make_shared<RequestHandler>(std::move(core));
            auto transport = std::make_unique<GrpcTransport>(request_handler);
            return std::make_unique<Controller>(request_handler, std::move(transport), port);
        }
        throw std::invalid_argument("Unknown controller type");
    }
}
