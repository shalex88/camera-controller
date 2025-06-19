#include <gtest/gtest.h>
/* Add your project include files here */
#include "data/CameraFactory.h"
#include "core/CoreFactory.h"
#include "api/ControllerFactory.h"
#include "common/Config/Config.h"

using namespace camera_service;
using namespace testing;

class ServiceSystemTests : public Test {
protected:
    void SetUp() override {
        EXPECT_NO_THROW(config = std::make_unique<Config>("../../config/config.yaml"));
        ASSERT_NE(nullptr, config);

        EXPECT_NO_THROW(camera = data::CameraFactory::createCamera(config->get("camera")));
        ASSERT_NE(nullptr, camera);

        EXPECT_NO_THROW(core = core::CoreFactory::createCore(config->get("camera"), std::move(camera)));
        ASSERT_NE(nullptr, core);

        EXPECT_NO_THROW(controller = camera_service::api::ControllerFactory::createController(config->get("api"),
            std::move(core)));
        ASSERT_NE(nullptr, controller);

        EXPECT_TRUE(controller->startAsync());
        EXPECT_TRUE(controller->isRunning());
    }

    std::unique_ptr<Config> config;
    std::unique_ptr<data::ICamera> camera;
    std::unique_ptr<core::ICore> core;
    std::unique_ptr<api::IController> controller;
};

TEST_F(ServiceSystemTests, CameraRequestResponse) {
    // TODO: send command from the client to setZoom and setFocus, get the response and check that the response is correct
    FAIL() << "Not implemented";
}

TEST_F(ServiceSystemTests, WorkingMonitoring) {
    // TODO: collect camera status
    FAIL() << "Not implemented";
}

TEST_F(ServiceSystemTests, WorkingLogging) {
    // TODO: collect logs from the service
    FAIL() << "Not implemented";
}
