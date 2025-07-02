#pragma once
#include <memory>
#include <string>

#include "core/ICore.h"
#include "api/Controller.h"

namespace camera_service::api {
    class ControllerFactory {
    public:
        static std::unique_ptr<Controller> createController(const std::string& controller_type,
                                                          const std::string& server_address,
                                                          std::unique_ptr<core::ICore> core);
    };
}
