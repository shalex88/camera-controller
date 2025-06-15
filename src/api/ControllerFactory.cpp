#include "ControllerFactory.h"

#include "api/GrpcController.h"
#include "api/TcpController.h"

namespace camera_service::api {
    std::unique_ptr<IController> ControllerFactory::createController(
        const std::string& controllerType, std::unique_ptr<core::ICore> core) {
        if (controllerType == "grpc") {
            return std::make_unique<GrpcController>(std::move(core));
        } else if (controllerType == "tcp") {
            return std::make_unique<TcpController>(std::move(core));
        }
        throw ControllerException("Unknown controller type");
    }
}
