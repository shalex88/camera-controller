#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include <chrono>
#include <memory>

#include "api/ApiControllerFactory.h"
#include "api/ApiController.h"
#include "core/CoreFactory.h"
#include "core/ICore.h"
#include "infrastructure/camera/CameraFactory.h"
#include "infrastructure/camera/hal/ICamera.h"
#include "common/logger/Logger.h"
#include "common/config/ConfigManager.h"
#include "common/types/Result.h"
#include "../../utils/GrpcClient.h"

using namespace camera_service;
using namespace testing;

class ServiceSystemTests : public Test {
protected:
    void SetUp() override {
        EXPECT_NO_THROW(config = std::make_unique<common::ConfigManager>("../../config/config.yaml"));
        ASSERT_NE(nullptr, config);

        CONFIGURE_LOGGER(config->getAppName(), config->getLogLevel());

        // Get configuration objects using the new typed API
        const auto& api_config_obj = config->getApiConfig();
        const auto& data_config_obj = config->getDataConfig();
        const auto& core_config_obj = config->getCoreConfig();

        api_config = api_config_obj.api;
        server_address_config = api_config_obj.server_address;
        camera_config = core_config_obj.camera;

    EXPECT_NO_THROW(camera = infrastructure::CameraFactory::createCamera(data_config_obj));
        ASSERT_NE(nullptr, camera);

    EXPECT_NO_THROW(core = core::CoreFactory::createCore(std::move(camera), core_config_obj));
        ASSERT_NE(nullptr, core);

    EXPECT_NO_THROW(service = camera_service::api::ApiControllerFactory::createController(std::move(core), api_config_obj));
        ASSERT_NE(nullptr, service);

        ASSERT_TRUE(service->startAsync().isSuccess());
        std::this_thread::sleep_for(1s);
        ASSERT_TRUE(service->isRunning());
    }

    std::unique_ptr<common::ConfigManager> config;
    std::unique_ptr<infrastructure::ICamera> camera;
    std::unique_ptr<core::ICore> core;
    std::unique_ptr<api::ApiController> service;
    std::string api_config;
    std::string server_address_config;
    std::string camera_config;
};

TEST_F(ServiceSystemTests, CameraRequestResponse) {
    std::cout << "Connecting to server at " << server_address_config << "\n";
    const auto channel = CreateChannel(server_address_config, grpc::InsecureChannelCredentials());
    const GrpcClient client(channel);

    constexpr types::zoom test_zoom = 1u;
    std::cout << "Test SetZoom " << test_zoom <<" and GetZoom" << "\n";
    ASSERT_TRUE(client.setZoom(test_zoom).isSuccess());

    const auto zoom_result = client.getZoom();
    ASSERT_TRUE(zoom_result.isSuccess());
    EXPECT_EQ(test_zoom, zoom_result.value());

    constexpr types::focus test_focus = 1u;
    std::cout << "Test SetFocus " << test_focus <<" and GetFocus" << "\n";
    ASSERT_TRUE(client.setFocus(test_focus).isSuccess());

    const auto focus_result = client.getFocus();
    ASSERT_TRUE(focus_result.isSuccess());
    EXPECT_EQ(test_focus, focus_result.value());
}
