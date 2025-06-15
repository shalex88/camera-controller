#pragma once
#include <memory>
#include <string>

#include "core/ICore.h"
#include "api/IController.h"

namespace camera_service::api {
    class ControllerFactory {
    public:
        static std::unique_ptr<IController> createController(const std::string& controllerType,
                                                                   std::unique_ptr<core::ICore> core);
    };
}
