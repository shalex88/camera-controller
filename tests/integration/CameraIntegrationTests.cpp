#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "data/CameraFactory.h"
#include "core/CoreFactory.h"
#include "common/Config/Config.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class CameraIntegrationTests : public Test {
protected:
    void SetUp() override {
        EXPECT_NO_THROW(config = std::make_unique<Config>("../../config/config.yaml"));
        ASSERT_NE(nullptr, config);

        EXPECT_NO_THROW(camera = data::CameraFactory::createCamera(config->get("camera")));
        ASSERT_NE(nullptr, camera);

        EXPECT_NO_THROW(core = core::CoreFactory::createCore(config->get("camera"), std::move(camera)));
        ASSERT_NE(nullptr, core);

        auto init_result = core->initialize();
        ASSERT_TRUE(init_result.isSuccess()) << "Failed to initialize core: " << init_result.error();
    }

    void TearDown() override {
        if (core) {
            const auto shutdown_result = core->shutdown();
            ASSERT_TRUE(shutdown_result.isSuccess()) << "Failed to shut down core: " << shutdown_result.error();
        }
    }

    std::unique_ptr<Config> config;
    std::unique_ptr<data::ICameraHal> camera;
    std::shared_ptr<core::ICore> core;
};

TEST_F(CameraIntegrationTests, CameraOperation) {
    const auto set_zoom_result = core->setZoom(1.5);
    ASSERT_TRUE(set_zoom_result.isSuccess()) << "Failed to set zoom: " << set_zoom_result.error();

    const auto get_zoom_result = core->getZoom();
    ASSERT_TRUE(get_zoom_result.isSuccess()) << "Failed to get zoom: " << get_zoom_result.error();
    EXPECT_EQ(get_zoom_result.value(), 1.5);

    const auto set_focus_result = core->setFocus(1.5);
    ASSERT_TRUE(set_focus_result.isSuccess()) << "Failed to set focus: " << set_focus_result.error();

    const auto get_focus_result = core->getFocus();
    ASSERT_TRUE(get_focus_result.isSuccess()) << "Failed to get focus: " << get_focus_result.error();
    EXPECT_EQ(get_focus_result.value(), 1.5);
}

TEST_F(CameraIntegrationTests, CameraReconnection) {
    const auto shutdown_result = core->shutdown();
    ASSERT_TRUE(shutdown_result.isSuccess()) << "Failed to shut down: " << shutdown_result.error();

    const auto init_result = core->initialize();
    ASSERT_TRUE(init_result.isSuccess()) << "Failed to initialize: " << init_result.error();
}
