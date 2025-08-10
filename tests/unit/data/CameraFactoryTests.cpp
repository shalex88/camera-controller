#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "data/CameraFactory.h"

using namespace camera_service;
using namespace testing;

class CameraFactoryTests : public Test {
protected:
    CameraFactoryTests() {
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "Data");
    }
    std::shared_ptr<LayerLogger> logger_impl_;
};

TEST_F(CameraFactoryTests, CreateNfovCameraSuccess) {
    const auto camera = data::CameraFactory::createCamera("nfov", logger_impl_);
    ASSERT_NE(nullptr, camera);
    ASSERT_TRUE(camera.get() != nullptr);
}

TEST_F(CameraFactoryTests, ThrowsOnUnknownType) {
    EXPECT_THROW(data::CameraFactory::createCamera("unknown", logger_impl_), std::invalid_argument);
}

TEST_F(CameraFactoryTests, ThrowsOnEmptyType) {
    EXPECT_THROW(data::CameraFactory::createCamera("", logger_impl_), std::invalid_argument);
}
