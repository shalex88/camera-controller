#include "ControllerFactory.h"
#include "api/Controller.h"
#include "api/GrpcAdapter.h"

namespace camera_service::api {
    std::unique_ptr<Controller> ControllerFactory::createController(
        const std::string& controller_type, const std::string& port, std::unique_ptr<core::ICore> core) {
        if (controller_type == "grpc") {
            auto adapter = std::make_unique<GrpcAdapter>();
            return std::make_unique<Controller>(std::move(core), std::move(adapter), port);
        }
        throw ControllerException("Unknown controller type");
    }
}
