#include <iostream>
#include <memory>

#include "data/CameraFactory.h"
#include "api/ControllerFactory.h"
#include "core/CoreFactory.h"
#include "common/Logger/Logger.h"
#include "common/Config/Config.h"

int main() {
    LOG_INFO("{} v{}.{}.{}{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH, APP_VERSION_DIRTY);

    // Cross-cutting concerns (common)
    auto config = std::make_unique<Config>("../config/config.yaml");

    // Data access layer
    auto camera = camera_service::data::CameraFactory::createCamera(config->get("camera"));

    // Business logic layer
    auto core = camera_service::core::CoreFactory::createCore(config->get("camera"), std::move(camera));

    // Presentation layer
    auto controller = camera_service::api::ControllerFactory::createController(config->get("api"), std::move(core));

    try {
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