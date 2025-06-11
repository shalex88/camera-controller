#include "CameraControllerFactory.h"

namespace camera_service::api {
    std::unique_ptr<ICameraController> CameraControllerFactory::createController(
        const std::string& controllerType, std::unique_ptr<core::ICore> core) {
        if (controllerType == "nfov") {
            return std::make_unique<CameraController>(std::move(core));
        }
        throw ControllerException("Unknown controller type");
    }
}
