#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "../../../src/infrastructure/camera/CameraFactory.h"
#include "common/config/ConfigManager.h"
#include "common/logger/SpdLogAdapter.h"
#include "infrastructure/camera/hal/ICamera.h"

using namespace camera_service;
using namespace testing;

class CameraFactoryTests : public Test {
protected:
    CameraFactoryTests() {
        logger_impl_ = std::make_shared<common::LayerLogger>(std::make_shared<SpdLogAdapter>(), "Data");
    }
    std::shared_ptr<common::LayerLogger> logger_impl_;
};

TEST_F(CameraFactoryTests, CreateSonyCameraSuccess) {
    common::DataConfig config;
    config.camera = "sony";  // Valid camera type
    config.device = "fake";  // Valid device type

    const auto camera = infrastructure::CameraFactory::createCamera(logger_impl_, config);
    ASSERT_NE(nullptr, camera);
    ASSERT_TRUE(camera.get() != nullptr);
}

TEST_F(CameraFactoryTests, ThrowsOnUnknownType) {
    common::DataConfig config;
    config.camera = "invalid_camera";  // Invalid camera type to trigger exception
    config.device = "fake";  // Valid device type

    EXPECT_THROW(infrastructure::CameraFactory::createCamera(logger_impl_, config), std::invalid_argument);
}

TEST_F(CameraFactoryTests, ThrowsOnEmptyType) {
    common::DataConfig config;
    config.camera = "";  // Empty camera type to trigger exception
    config.device = "fake";  // Valid device type

    EXPECT_THROW(infrastructure::CameraFactory::createCamera(logger_impl_, config), std::invalid_argument);
}
