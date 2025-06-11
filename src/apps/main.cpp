#include <iostream>
#include <memory>

#include "data/CameraFactory.h"
#include "core/CameraCore.h"
#include "api/CameraController.h"
#include "common/Logger/Logger.h"
// #include "common/Logger/Config.h"

int main() {
    LOG_INFO("{} v{}.{}.{}{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH, APP_VERSION_DIRTY);

    // Cross-cutting concerns (common)
    // auto config = std::make_shared<Config>();
    // try {
    //     config->load_from_file("config/config.json");
    // } catch (const std::exception& e) {
    //     LOG_ERROR("Error loading configuration: {}", e.what());
    //     return EXIT_FAILURE;
    // }

    // Data access layer (camera)
    auto camera = camera_service::data::CameraFactory::createCamera("nfov");

    // Business logic layer (core)
    auto core = std::make_unique<camera_service::core::CameraCore>(std::move(camera));

    // Presentation layer (grpc)
    auto controller = std::make_unique<camera_service::api::CameraController>(std::move(core));

    try {
        LOG_INFO("Starting controller...");
        if (!controller->start()) {
            LOG_ERROR("Controller failed");
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error during startup: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
