#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <CLI/CLI.hpp>

#include "api/ApiController.h"
#include "api/ApiControllerFactory.h"
#include "common/config/ConfigManager.h"
#include "common/logger/Logger.h"
#include "core/CoreFactory.h"
#include "core/ICore.h"
#include "infrastructure/camera/CameraFactory.h"
#include "infrastructure/camera/hal/ICamera.h"

std::string parseInputArgs(const int argc, char* argv[]) {
    CLI::App app{"A camera control service", APP_NAME};

    std::string config_file = "../config/config.yaml";
    bool show_version = false;

    app.add_flag("-v,--version", show_version, "Show version information");
    app.add_option("-c,--config", config_file, "Configuration file path")->check(CLI::ExistingFile);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        std::exit(app.exit(e));
    }

    if (show_version) {
        std::cout << APP_NAME << " v" << APP_VERSION_MAJOR << "." << APP_VERSION_MINOR << "." << APP_VERSION_PATCH <<
            APP_VERSION_DIRTY << "\n";
        std::exit(EXIT_SUCCESS);
    }

    return config_file;
}

int main(const int argc, char* argv[]) {
    const auto config_file = parseInputArgs(argc, argv);

    try {
        const auto config = std::make_unique<camera_service::common::ConfigManager>(config_file);

        CONFIGURE_GLOBAL_LOGGER(config->getAppName(), config->getLogLevel());

        LOG_INFO("{} v{}.{}.{}{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH,
                 APP_VERSION_DIRTY);

        auto camera = camera_service::infrastructure::CameraFactory::createCamera(config->getDataConfig());
        auto core = camera_service::core::CoreFactory::createCore(std::move(camera), config->getCoreConfig());
        const auto api_controller = camera_service::api::ApiControllerFactory::createController(
            std::move(core), config->getApiConfig());

        if (const auto app = api_controller->startAsync(); app.isError()) {
            LOG_ERROR("Shutting down due to startup error: {}", app.error());
            return EXIT_FAILURE;
        }

        LOG_INFO("Running...");

        while (api_controller->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Startup error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
