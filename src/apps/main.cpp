#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "data/CameraFactory.h"
#include "api/ApiControllerFactory.h"
#include "api/ApiController.h"
#include "core/CoreFactory.h"
#include "common/Logger/Logger.h"
#include "common/Config/Config.h"

int main() {
    LOG_INFO("{} v{}.{}.{}{}", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH, APP_VERSION_DIRTY);

    try {
        auto logger_impl = std::make_shared<SpdLogAdapter>();
        const auto api_logger = std::make_shared<LayerLogger>(logger_impl, "API");
        const auto core_logger = std::make_shared<LayerLogger>(logger_impl, "CORE");
        const auto data_logger = std::make_shared<LayerLogger>(logger_impl, "DATA");

        //TODO: add default values if not found in config
        const auto config = std::make_unique<Config>("../config/config.yaml");
        const auto api_config = config->get("api");
        const auto port_config = config->get("server_address");
        const auto camera_config = config->get("camera");
        const auto device = config->get("device");

        if (const auto log_level = config->get("log_level"); !log_level.empty()) {
            api_logger->setLogLevel(log_level);
            core_logger->setLogLevel(log_level);
            data_logger->setLogLevel(log_level);
            SET_LOG_LEVEL(log_level);
        }

        auto camera = camera_service::data::CameraFactory::createCamera(camera_config, data_logger, device);

        auto core = camera_service::core::CoreFactory::createCore(camera_config, std::move(camera), core_logger);

        const auto api_controller = camera_service::api::ApiControllerFactory::createController(
            api_config, port_config, std::move(core), api_logger);

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