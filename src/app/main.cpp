#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include <CLI/CLI.hpp>

#include "data/CameraFactory.h"
#include "api/ApiControllerFactory.h"
#include "api/ApiController.h"
#include "core/CoreFactory.h"
#include "common/Logger/Logger.h"
#include "common/Config/ConfigManager.h"

std::string parseInputArgs(const int argc, char* argv[]) {
    CLI::App app{"A camera control service", APP_NAME};

    // Defaults
    std::string config_file = "../config/config.yaml";
    bool show_version = false;

    app.add_flag("-v,--version", show_version, "Show version information");
    app.add_option("-c,--config", config_file, "Configuration file path")
        ->check(CLI::ExistingFile);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        std::exit(app.exit(e));
    }

    if (show_version) {
        std::cout << APP_NAME << " v" << APP_VERSION_MAJOR << "."
                  << APP_VERSION_MINOR << "." << APP_VERSION_PATCH
                  << APP_VERSION_DIRTY << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    return config_file;
}

int main(const int argc, char* argv[]) {
    const auto config_file = parseInputArgs(argc, argv);

    try {
        const auto config = std::make_unique<ConfigManager>(config_file);

        CONFIGURE_GLOBAL_LOGGER(config->getAppName(), config->getLogLevel());

        auto scoped_logger_impl = std::make_shared<SpdLogAdapter>(config->getAppName());
        const auto api_logger = std::make_shared<LayerLogger>(scoped_logger_impl, "API", config->getLogLevel());
        const auto core_logger = std::make_shared<LayerLogger>(scoped_logger_impl, "CORE", config->getLogLevel());
        const auto data_logger = std::make_shared<LayerLogger>(scoped_logger_impl, "DATA", config->getLogLevel());

        LOG_INFO("{} v{}.{}.{}{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH, APP_VERSION_DIRTY);

        auto camera = camera_service::data::CameraFactory::createCamera(data_logger, config->getDataConfig());
        auto core = camera_service::core::CoreFactory::createCore(std::move(camera), core_logger, config->getCoreConfig());
        const auto api_controller = camera_service::api::ApiControllerFactory::createController(std::move(core), api_logger, config->getApiConfig());

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