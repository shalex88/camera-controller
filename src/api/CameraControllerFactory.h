#pragma once
#include <memory>
#include <string>

#include "core/ICore.h"
#include "api/ICameraController.h"

namespace camera_service::api {
    class CameraControllerFactory {
    public:
        static std::unique_ptr<ICameraController> createController(const std::string& controllerType,
                                                                   std::unique_ptr<core::ICore> core);
    };
}
