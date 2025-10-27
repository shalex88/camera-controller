#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "../../../src/infrastructure/camera/CameraFactory.h"
#include "common/config/ConfigManager.h"
#include "infrastructure/camera/hal/ICamera.h"

using namespace camera_service;
using namespace testing;

TEST(CameraFactoryTests, CreateSonyCameraSuccess) {
    common::DataConfig config;
    config.camera = "sony";  // Valid camera type
    config.device = "fake";  // Valid device type

    const auto camera = infrastructure::CameraFactory::createCamera(config);
    ASSERT_NE(nullptr, camera);
    ASSERT_TRUE(camera.get() != nullptr);
}

TEST(CameraFactoryTests, ThrowsOnUnknownType) {
    common::DataConfig config;
    config.camera = "invalid_camera";  // Invalid camera type to trigger exception
    config.device = "fake";  // Valid device type

    EXPECT_THROW(infrastructure::CameraFactory::createCamera(config), std::invalid_argument);
}

TEST(CameraFactoryTests, ThrowsOnEmptyType) {
    common::DataConfig config;
    config.camera = "";  // Empty camera type to trigger exception
    config.device = "fake";  // Valid device type

    EXPECT_THROW(infrastructure::CameraFactory::createCamera(config), std::invalid_argument);
}
