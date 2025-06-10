#include <iostream>
#include <memory>

#include "data/camera.hpp"
#include "core/core.hpp"
#include "api/controller.hpp"
#include "common/logger.hpp"
#include "common/config.hpp"

int main() {
    std::cout << APP_NAME << " v" << APP_VERSION_MAJOR << "." << APP_VERSION_MINOR << "." << APP_VERSION_PATCH << APP_VERSION_DIRTY << std::endl;

    //TODO: cross-cutting concerns (common)
    auto config = std::make_shared<Config>();
    try {
        config->load_from_file("config/config.json");
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    auto logger = std::make_shared<Logger>(config);

    //TODO: create data access layer (camera)
    logger->info("Initializing data access layer...");
    auto camera = std::make_unique<Camera>(logger, config);
    logger->info("Camera initialized");

    //TODO: create business logic layer (core)
    logger->info("Initializing business logic layer...");
    auto core = std::make_unique<Core>(std::move(camera), logger, config);
    logger->info("Core initialized");

    //TODO: create presentation layer (grpc)
    logger->info("Initializing presentation layer...");
    auto controller = std::make_unique<Controller>(std::move(core), logger, config);
    logger->info("Controller initialized");

    try {
        logger->info("Starting controller...");
        controller->start();
        logger->info("Controller started");
    } catch (const std::exception& e) {
        logger->error("Error during startup: {}", e.what());
        return EXIT_FAILURE;
    }

    waitForTermination();

    try {
        logger->info("Stopping controller...");
        controller->stop();
        logger->info("Controller stopped");
    } catch (const std::exception& e) {
        logger->error("Error during shutdown: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
