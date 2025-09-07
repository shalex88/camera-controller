#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "data/CameraFactory.h"
#include "api/ApiControllerFactory.h"
#include "api/ApiController.h"
#include "core/CoreFactory.h"
#include "common/Logger/Logger.h"
#include "common/Config/ConfigManager.h"

int main() {
    LOG_INFO("{} v{}.{}.{}{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH, APP_VERSION_DIRTY);

    try {
        const auto config = std::make_unique<ConfigManager>("../config/config.yaml");

        SET_LOG_LEVEL(config->getLogLevel());
        auto logger_impl = std::make_shared<SpdLogAdapter>();
        const auto api_logger = std::make_shared<LayerLogger>(logger_impl, "API", config->getLogLevel());
        const auto core_logger = std::make_shared<LayerLogger>(logger_impl, "CORE", config->getLogLevel());
        const auto data_logger = std::make_shared<LayerLogger>(logger_impl, "DATA", config->getLogLevel());

        auto camera = camera_service::data::CameraFactory::createCamera(data_logger, config->getDataConfig());

        auto core = camera_service::core::CoreFactory::createCore(std::move(camera), core_logger, config->getCoreConfig());

        const auto api_controller = camera_service::api::ApiControllerFactory::createController(std::move(core), api_logger, config->getApiConfig());

        if (api_controller->startAsync().isError()) {
            LOG_ERROR("Failed to start API controller");
            return EXIT_FAILURE;
        }

        LOG_INFO("Camera service started successfully");

        while (api_controller->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Startup error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}