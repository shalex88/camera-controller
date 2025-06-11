#include <iostream>
#include <memory>

#include "data/Camera.h"
#include "core/Core.h"
#include "api/CameraController.h"
#include "common/Logger/Logger.h"
// #include "common/Logger/Config.h"

int main() {
    LOG_INFO("{} v{}.{}.{}{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH, APP_VERSION_DIRTY);

    //TODO: cross-cutting concerns (common)
    // auto config = std::make_shared<Config>();
    // try {
    //     config->load_from_file("config/config.json");
    // } catch (const std::exception& e) {
    //     std::cerr << "Error loading configuration: " << e.what() << std::endl;
    //     return EXIT_FAILURE;
    // }

    //TODO: create data access layer (camera)
    auto camera = std::make_unique<nfov::data::Camera>();

    //TODO: create business logic layer (core)
    auto core = std::make_unique<nfov::core::Core>(std::move(camera));

    //TODO: create presentation layer (grpc)
    auto controller = std::make_unique<nfov::api::CameraController>(std::move(core));

    try {
        LOG_INFO("Starting controller...");
        if (controller->start() != EXIT_SUCCESS) {
            LOG_INFO("Controller stopped");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error during startup: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
