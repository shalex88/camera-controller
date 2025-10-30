#include "app/Application.h"
#include "common/logger/Logger.h"

int main(const int argc, char* argv[]) {
    camera_service::app::Application app(argc, argv);

    if (const auto result = app.initialize(); result.isError()) {
        LOG_ERROR("Initialization failed: {}", result.error());
        return EXIT_FAILURE;
    }

    if (const auto result = app.start(); result.isError()) {
        LOG_ERROR("Failed to start: {}", result.error());
        return EXIT_FAILURE;
    }

    app.run();

    if (const auto result = app.stop(); result.isError()) {
        LOG_ERROR("Shutdown error: {}", result.error());
        return EXIT_FAILURE;
    }

    LOG_INFO("Stopped gracefully");
    return EXIT_SUCCESS;
}
