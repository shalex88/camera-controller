#include <iostream>
#include <memory>

#include "data/CameraFactory.h"
#include "api/ControllerFactory.h"
#include "core/CoreFactory.h"
#include "common/Logger/Logger.h"
#include "common/Config/Config.h"

int main() {
    LOG_INFO("{} v{}.{}.{}{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH, APP_VERSION_DIRTY);

    try {
        // Cross-cutting concerns (common)
        const auto config = std::make_unique<Config>("../config/config.yaml");
        const auto api_config = config->get("api");
        const auto port_config = config->get("port");
        const auto camera_config = config->get("camera");

        // Data access layer
        auto camera = camera_service::data::CameraFactory::createCamera(camera_config);

        // Business logic layer
        auto core = camera_service::core::CoreFactory::createCore(camera_config, std::move(camera));

        // Presentation layer
        const auto controller = camera_service::api::ControllerFactory::createController(api_config, port_config, std::move(core));

        controller->startAsync();
        while (controller->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error during startup: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}