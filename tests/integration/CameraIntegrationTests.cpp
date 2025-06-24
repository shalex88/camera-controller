#include <gtest/gtest.h>
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

        auto initResult = core->initialize();
        ASSERT_TRUE(initResult.isSuccess()) << "Failed to initialize core: " << initResult.error();
    }

    void TearDown() override {
        if (core) {
            auto shutdownResult = core->shutdown();
            EXPECT_TRUE(shutdownResult.isSuccess()) << "Failed to shutdown core: " << shutdownResult.error();
        }
    }

    std::unique_ptr<Config> config;
    std::unique_ptr<data::ICamera> camera;
    std::shared_ptr<core::ICore> core;
};

TEST_F(CameraIntegrationTests, CameraOperation) {
    auto setZoomResult = core->setZoom(1.5);
    EXPECT_TRUE(setZoomResult.isSuccess()) << "Failed to set zoom: " << setZoomResult.error();

    auto getZoomResult = core->getZoom();
    ASSERT_TRUE(getZoomResult.isSuccess()) << "Failed to get zoom: " << getZoomResult.error();
    EXPECT_EQ(getZoomResult.value(), 1.5);

    auto setFocusResult = core->setFocus(1.5);
    EXPECT_TRUE(setFocusResult.isSuccess()) << "Failed to set focus: " << setFocusResult.error();

    auto getFocusResult = core->getFocus();
    ASSERT_TRUE(getFocusResult.isSuccess()) << "Failed to get focus: " << getFocusResult.error();
    EXPECT_EQ(getFocusResult.value(), 1.5);
}

TEST_F(CameraIntegrationTests, CameraReconnection) {
    auto shutdownResult = core->shutdown();
    EXPECT_TRUE(shutdownResult.isSuccess()) << "Failed to shutdown: " << shutdownResult.error();

    auto initResult = core->initialize();
    EXPECT_TRUE(initResult.isSuccess()) << "Failed to initialize: " << initResult.error();
}
