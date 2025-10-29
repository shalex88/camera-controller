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
    config.camera = "sony";
    config.device = "fake";

    const auto camera = infrastructure::CameraFactory::createCamera(config);
    ASSERT_NE(nullptr, camera);
    ASSERT_TRUE(camera.get() != nullptr);
}

TEST(CameraFactoryTests, ThrowsOnUnknownType) {
    common::DataConfig config;
    config.camera = "invalid_camera";
    config.device = "fake";

    EXPECT_THROW(infrastructure::CameraFactory::createCamera(config), std::invalid_argument);
}

TEST(CameraFactoryTests, ThrowsOnEmptyType) {
    common::DataConfig config;
    config.camera = "";
    config.device = "fake";

    EXPECT_THROW(infrastructure::CameraFactory::createCamera(config), std::invalid_argument);
}
