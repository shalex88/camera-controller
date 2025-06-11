#pragma once
#include <memory>
#include <string>

#include "CameraController.h"
#include "core/CameraCore.h"

namespace camera_service::api {
    class CameraControllerFactory {
    public:
        static std::unique_ptr<ICameraController> createController(const std::string& controllerType,
                                                                   std::unique_ptr<core::ICore> core);
    };
}
