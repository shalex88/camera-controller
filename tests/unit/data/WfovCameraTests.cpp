#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "data/WfovCamera.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class WfovCameraTests : public Test {
protected:
    void SetUp() override {
        camera = std::make_unique<data::WfovCamera>();
    }

    std::unique_ptr<data::WfovCamera> camera;
};

TEST_F(WfovCameraTests, CanBeConstructed) {
    auto camera = std::make_unique<data::WfovCamera>();
    ASSERT_NE(nullptr, camera);
}

TEST_F(WfovCameraTests, InitiallyNotConnected) {
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(WfovCameraTests, ConnectDisconnect) {
    auto connectResult = camera->connect();
    EXPECT_TRUE(connectResult.isSuccess()) << "Failed to connect: " << connectResult.error();
    EXPECT_TRUE(camera->isConnected());

    auto disconnectResult = camera->disconnect();
    EXPECT_TRUE(disconnectResult.isSuccess()) << "Failed to disconnect: " << disconnectResult.error();
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(WfovCameraTests, DisconnectWhenNotConnected) {
    auto result = camera->disconnect();
    EXPECT_TRUE(result.isError());
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(WfovCameraTests, SetZoomWhenNotConnected) {
    auto result = camera->setZoom(2.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot set zoom: WFOV Camera not connected");
}

TEST_F(WfovCameraTests, GetZoomWhenNotConnected) {
    auto result = camera->getZoom();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot get zoom: WFOV Camera not connected");
}

TEST_F(WfovCameraTests, SetFocusWhenNotConnected) {
    auto result = camera->setFocus(1.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot set focus: WFOV Camera not connected");
}

TEST_F(WfovCameraTests, GetFocusWhenNotConnected) {
    auto result = camera->getFocus();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot get focus: WFOV Camera not connected");
}

TEST_F(WfovCameraTests, ZoomOperations) {
    auto connectResult = camera->connect();
    ASSERT_TRUE(connectResult.isSuccess()) << "Failed to connect: " << connectResult.error();

    auto setResult = camera->setZoom(2.0);
    EXPECT_TRUE(setResult.isSuccess()) << "Failed to set zoom: " << setResult.error();

    auto getResult = camera->getZoom();
    ASSERT_TRUE(getResult.isSuccess()) << "Failed to get zoom: " << getResult.error();
    EXPECT_DOUBLE_EQ(2.0, getResult.value());
}

TEST_F(WfovCameraTests, FocusOperations) {
    auto connectResult = camera->connect();
    ASSERT_TRUE(connectResult.isSuccess()) << "Failed to connect: " << connectResult.error();

    auto setResult = camera->setFocus(1.5);
    EXPECT_TRUE(setResult.isSuccess()) << "Failed to set focus: " << setResult.error();

    auto getResult = camera->getFocus();
    ASSERT_TRUE(getResult.isSuccess()) << "Failed to get focus: " << getResult.error();
    EXPECT_DOUBLE_EQ(1.5, getResult.value());
}

TEST_F(WfovCameraTests, InvalidZoomValue) {
    auto connectResult = camera->connect();
    ASSERT_TRUE(connectResult.isSuccess()) << "Failed to connect: " << connectResult.error();

    auto result = camera->setZoom(-1.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Invalid zoom level: Value must be greater than zero");
}
