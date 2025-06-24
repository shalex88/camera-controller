#include "ControllerFactory.h"
#include "api/Controller.h"
#include "api/GrpcTransport.h"

namespace camera_service::api {
    std::unique_ptr<Controller> ControllerFactory::createController(
        const std::string& controller_type, const std::string& port, std::unique_ptr<core::ICore> core) {
        if (controller_type == "grpc") {
            auto transport = std::make_unique<GrpcTransport>();
            return std::make_unique<Controller>(std::move(core), std::move(transport), port);
        }
        throw std::invalid_argument("Unknown controller type");
    }
}
