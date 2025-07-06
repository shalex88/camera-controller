#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include <chrono>
#include <memory>

#include "api/ControllerFactory.h"
#include "api/Controller.h"
#include "core/CoreFactory.h"
#include "data/CameraFactory.h"
#include "common/Config/Config.h"
#include "common/types/Result.h"
#include "../../utils/GrpcClient.h"

using namespace camera_service;
using namespace testing;

class ServiceSystemTests : public Test {
protected:
    void SetUp() override {
        EXPECT_NO_THROW(config = std::make_unique<Config>("../../config/config.yaml"));
        ASSERT_NE(nullptr, config);
        EXPECT_NO_THROW(api_config = config->get("api"));
        EXPECT_NO_THROW(server_address_config = config->get("server_address"));
        EXPECT_NO_THROW(camera_config = config->get("camera"));

        EXPECT_NO_THROW(camera = data::CameraFactory::createCamera(camera_config));
        ASSERT_NE(nullptr, camera);

        EXPECT_NO_THROW(core = core::CoreFactory::createCore(camera_config, std::move(camera)));
        ASSERT_NE(nullptr, core);

        EXPECT_NO_THROW(service = camera_service::api::ControllerFactory::createController(api_config, server_address_config,
            std::move(core)));
        ASSERT_NE(nullptr, service);

        ASSERT_TRUE(service->startAsync().isSuccess());
        std::this_thread::sleep_for(1s);
        ASSERT_TRUE(service->isRunning());
    }

    std::unique_ptr<Config> config;
    std::unique_ptr<data::ICamera> camera;
    std::unique_ptr<core::ICore> core;
    std::unique_ptr<api::Controller> service;
    std::string api_config;
    std::string server_address_config;
    std::string camera_config;
};

TEST_F(ServiceSystemTests, CameraRequestResponse) {
    std::cout << "Connecting to server at " << server_address_config << std::endl;
    const auto channel = CreateChannel(server_address_config, grpc::InsecureChannelCredentials());
    const GrpcClient client(channel);

    constexpr double test_zoom = 2.5;
    std::cout << "Test SetZoom " << test_zoom <<" and GetZoom" << std::endl;
    ASSERT_TRUE(client.setZoom(test_zoom).isSuccess());

    const auto zoom_result = client.getZoom();
    ASSERT_TRUE(zoom_result.isSuccess());
    EXPECT_DOUBLE_EQ(test_zoom, zoom_result.value());

    constexpr double test_focus = 1.8;
    std::cout << "Test SetFocus " << test_focus <<" and GetFocus" << std::endl;
    ASSERT_TRUE(client.setFocus(test_focus).isSuccess());

    const auto focus_result = client.getFocus();
    ASSERT_TRUE(focus_result.isSuccess());
    EXPECT_DOUBLE_EQ(test_focus, focus_result.value());
}
