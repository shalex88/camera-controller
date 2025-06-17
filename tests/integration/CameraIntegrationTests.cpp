#include <gtest/gtest.h>
/* Add your project include files here */
#include "data/CameraFactory.h"
#include "core/CoreFactory.h"
#include "common/Config/Config.h"

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

        EXPECT_TRUE(core->initialize());
    }
    std::unique_ptr<Config> config;
    std::unique_ptr<data::ICamera> camera;
    std::shared_ptr<core::ICore> core;
};

TEST_F(CameraIntegrationTests, CameraOperation) {
    EXPECT_NO_THROW(core->setZoom(1.5));
    EXPECT_EQ(core->getZoom(), 1.5);
    EXPECT_NO_THROW(core->setFocus(1.5));
    EXPECT_EQ(core->getFocus(), 1.5);
}

TEST_F(CameraIntegrationTests, CameraReconnection) {
    EXPECT_NO_THROW(core->shutdown());
    EXPECT_TRUE(core->initialize());
}
