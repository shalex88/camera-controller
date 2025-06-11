#include "CameraControllerFactory.h"

#include "api/GrpcCameraController.h"
#include "api/TcpCameraController.h"

namespace camera_service::api {
    std::unique_ptr<ICameraController> CameraControllerFactory::createController(
        const std::string& controllerType, std::unique_ptr<core::ICore> core) {
        if (controllerType == "grpc") {
            return std::make_unique<GrpcCameraController>(std::move(core));
        } else if (controllerType == "tcp") {
            return std::make_unique<TcpCameraController>(std::move(core));
        }
        throw ControllerException("Unknown controller type");
    }
}
