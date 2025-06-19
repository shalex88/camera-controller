#include "ControllerFactory.h"
#include "api/ApiController.h"
#include "api/ApiAdapterFactory.h"

namespace camera_service::api {
    std::unique_ptr<IController> ControllerFactory::createController(
        const std::string& controller_type, const std::string& port, std::unique_ptr<core::ICore> core) {
        if (controller_type == "grpc") {
            auto adapter = ApiAdapterFactory::createAdapter(controller_type);
            return std::make_unique<ApiController>(std::move(core), std::move(adapter), port);
        }
        throw ControllerException("Unknown controller type");
    }
}
