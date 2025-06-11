#include <iostream>
#include <memory>

#include "data/Camera.h"
#include "core/Core.h"
#include "api/Controller.h"
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
    LOG_INFO("Initializing data access layer...");
    auto camera = std::make_unique<Camera>();
    LOG_INFO("Camera initialized");

    //TODO: create business logic layer (core)
    LOG_INFO("Initializing business logic layer...");
    auto core = std::make_unique<Core>(std::move(camera));
    LOG_INFO("Core initialized");

    //TODO: create presentation layer (grpc)
    LOG_INFO("Initializing presentation layer...");
    auto controller = std::make_unique<Controller>(std::move(core));
    LOG_INFO("Controller initialized");

    try {
        LOG_INFO("Starting controller...");
        if (controller->start() != EXIT_SUCCESS) {
            LOG_INFO("Controller started");
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error during startup: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
