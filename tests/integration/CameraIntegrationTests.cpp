#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "data/CameraFactory.h"
#include "core/CoreFactory.h"
#include "common/Config/ConfigManager.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class CameraIntegrationTests : public Test {
protected:
    void SetUp() override {
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "");
        EXPECT_NO_THROW(config = std::make_unique<ConfigManager>("../../config/config.yaml"));
        ASSERT_NE(nullptr, config);

        // Create DataConfig for camera creation
        DataConfig data_config;
        data_config.camera = "nfov";  // Use valid camera type
        data_config.device = "/dev/uio0";  // Use UIO device

        EXPECT_NO_THROW(camera = data::CameraFactory::createCamera(logger_impl_, data_config));
        ASSERT_NE(nullptr, camera);

        // Create CoreConfig for core creation
        CoreConfig core_config;
        core_config.camera = "nfov";  // Use same camera type as data config

        EXPECT_NO_THROW(core = core::CoreFactory::createCore(std::move(camera), logger_impl_, core_config));
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

    std::unique_ptr<ConfigManager> config;
    std::unique_ptr<data::ICameraHal> camera;
    std::shared_ptr<core::ICore> core;
    std::shared_ptr<LayerLogger> logger_impl_;
};

TEST_F(CameraIntegrationTests, CameraOperation) {
    const auto set_zoom_result = core->setZoom(2u);
    ASSERT_TRUE(set_zoom_result.isSuccess()) << "Failed to set zoom: " << set_zoom_result.error();

    const auto get_zoom_result = core->getZoom();
    ASSERT_TRUE(get_zoom_result.isSuccess()) << "Failed to get zoom: " << get_zoom_result.error();
    EXPECT_EQ(get_zoom_result.value(), 2u);

    const auto set_focus_result = core->setFocus(3u);
    ASSERT_TRUE(set_focus_result.isSuccess()) << "Failed to set focus: " << set_focus_result.error();

    const auto get_focus_result = core->getFocus();
    ASSERT_TRUE(get_focus_result.isSuccess()) << "Failed to get focus: " << get_focus_result.error();
    EXPECT_EQ(get_focus_result.value(), 3u);
}

TEST_F(CameraIntegrationTests, CameraReconnection) {
    const auto shutdown_result = core->shutdown();
    ASSERT_TRUE(shutdown_result.isSuccess()) << "Failed to shut down: " << shutdown_result.error();

    const auto init_result = core->initialize();
    ASSERT_TRUE(init_result.isSuccess()) << "Failed to initialize: " << init_result.error();
}
