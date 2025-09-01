#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include <chrono>
#include <memory>

#include "api/ApiControllerFactory.h"
#include "api/ApiController.h"
#include "core/CoreFactory.h"
#include "data/CameraFactory.h"
#include "common/Config/ConfigManager.h"
#include "common/types/Result.h"
#include "../../utils/GrpcClient.h"

using namespace camera_service;
using namespace testing;

class ServiceSystemTests : public Test {
protected:
    void SetUp() override {
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "");

        EXPECT_NO_THROW(config = std::make_unique<ConfigManager>("../../config/config.yaml"));
        ASSERT_NE(nullptr, config);

        // Get configuration objects using the new typed API
        const auto& api_config_obj = config->getApiConfig();
        const auto& data_config_obj = config->getDataConfig();
        const auto& core_config_obj = config->getCoreConfig();

        api_config = api_config_obj.api;
        server_address_config = api_config_obj.server_address;
        camera_config = core_config_obj.camera;

        EXPECT_NO_THROW(camera = data::CameraFactory::createCamera(logger_impl_, data_config_obj));
        ASSERT_NE(nullptr, camera);

        EXPECT_NO_THROW(core = core::CoreFactory::createCore(std::move(camera), logger_impl_, core_config_obj));
        ASSERT_NE(nullptr, core);

        EXPECT_NO_THROW(service = camera_service::api::ApiControllerFactory::createController(std::move(core), logger_impl_, api_config_obj));
        ASSERT_NE(nullptr, service);

        ASSERT_TRUE(service->startAsync().isSuccess());
        std::this_thread::sleep_for(1s);
        ASSERT_TRUE(service->isRunning());
    }

    std::unique_ptr<ConfigManager> config;
    std::unique_ptr<data::ICameraHal> camera;
    std::unique_ptr<core::ICore> core;
    std::unique_ptr<api::ApiController> service;
    std::string api_config;
    std::string server_address_config;
    std::string camera_config;
    std::shared_ptr<LayerLogger> logger_impl_;
};

TEST_F(ServiceSystemTests, CameraRequestResponse) {
    std::cout << "Connecting to server at " << server_address_config << std::endl;
    const auto channel = CreateChannel(server_address_config, grpc::InsecureChannelCredentials());
    const GrpcClient client(channel);

    constexpr types::zoom test_zoom = 1u;
    std::cout << "Test SetZoom " << test_zoom <<" and GetZoom" << std::endl;
    ASSERT_TRUE(client.setZoom(test_zoom).isSuccess());

    const auto zoom_result = client.getZoom();
    ASSERT_TRUE(zoom_result.isSuccess());
    EXPECT_EQ(test_zoom, zoom_result.value());

    constexpr types::focus test_focus = 1u;
    std::cout << "Test SetFocus " << test_focus <<" and GetFocus" << std::endl;
    ASSERT_TRUE(client.setFocus(test_focus).isSuccess());

    const auto focus_result = client.getFocus();
    ASSERT_TRUE(focus_result.isSuccess());
    EXPECT_EQ(test_focus, focus_result.value());
}
