#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "data/CameraFactory.h"

#include "data/NfovCamera.h"
#include "data/WfovCamera.h"

using namespace camera_service;
using namespace testing;

TEST(CameraFactoryTests, CreateNfovCameraSuccess) {
    auto camera = data::CameraFactory::createCamera("nfov");
    ASSERT_NE(nullptr, camera);
    EXPECT_TRUE(dynamic_cast<data::NfovCamera*>(camera.get()) != nullptr);
}

TEST(CameraFactoryTests, CreateWfovCameraSuccess) {
    auto camera = data::CameraFactory::createCamera("wfov");
    ASSERT_NE(nullptr, camera);
    EXPECT_TRUE(dynamic_cast<data::WfovCamera*>(camera.get()) != nullptr);
}

TEST(CameraFactoryTests, ThrowsOnUnknownType) {
    EXPECT_THROW(data::CameraFactory::createCamera("unknown"), std::invalid_argument);
}

TEST(CameraFactoryTests, ThrowsOnEmptyType) {
    EXPECT_THROW(data::CameraFactory::createCamera(""), std::invalid_argument);
}
