#include "ControllerFactory.h"

#include "api/GrpcController.h"

namespace camera_service::api {
    std::unique_ptr<IController> ControllerFactory::createController(
        const std::string& controller_type, std::unique_ptr<core::ICore> core) {
        if (controller_type == "grpc") {
            return std::make_unique<GrpcController>(std::move(core));
        }
        throw ControllerException("Unknown controller type");
    }
}
