#pragma once
#include <memory>
#include <string>

#include "core/ICore.h"
#include "api/ApiController.h"

namespace camera_service::api {
    class ApiControllerFactory {
    public:
        static std::unique_ptr<ApiController> createController(const std::string& controller_type,
                                                          const std::string& server_address,
                                                          std::unique_ptr<core::ICore> core);
    };
}
