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
    const auto camera = std::make_unique<data::WfovCamera>();
    ASSERT_NE(nullptr, camera);
}

TEST_F(WfovCameraTests, InitiallyNotConnected) {
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(WfovCameraTests, ConnectDisconnect) {
    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess()) << "Failed to connect: " << connect_result.error();
    EXPECT_TRUE(camera->isConnected());

    const auto disconnect_result = camera->disconnect();
    EXPECT_TRUE(disconnect_result.isSuccess()) << "Failed to disconnect: " << disconnect_result.error();
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(WfovCameraTests, DisconnectWhenNotConnected) {
    const auto result = camera->disconnect();
    EXPECT_TRUE(result.isError());
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(WfovCameraTests, SetZoomWhenNotConnected) {
    const auto result = camera->setZoom(2.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot set zoom: WFOV Camera not connected");
}

TEST_F(WfovCameraTests, GetZoomWhenNotConnected) {
    const auto result = camera->getZoom();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot get zoom: WFOV Camera not connected");
}

TEST_F(WfovCameraTests, SetFocusWhenNotConnected) {
    const auto result = camera->setFocus(1.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot set focus: WFOV Camera not connected");
}

TEST_F(WfovCameraTests, GetFocusWhenNotConnected) {
    const auto result = camera->getFocus();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot get focus: WFOV Camera not connected");
}

TEST_F(WfovCameraTests, ZoomOperations) {
    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess()) << "Failed to connect: " << connect_result.error();

    const auto set_result = camera->setZoom(2.0);
    EXPECT_TRUE(set_result.isSuccess()) << "Failed to set zoom: " << set_result.error();

    const auto get_result = camera->getZoom();
    ASSERT_TRUE(get_result.isSuccess()) << "Failed to get zoom: " << get_result.error();
    EXPECT_DOUBLE_EQ(2.0, get_result.value());
}

TEST_F(WfovCameraTests, FocusOperations) {
    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess()) << "Failed to connect: " << connect_result.error();

    const auto set_result = camera->setFocus(1.5);
    EXPECT_TRUE(set_result.isSuccess()) << "Failed to set focus: " << set_result.error();

    const auto get_result = camera->getFocus();
    ASSERT_TRUE(get_result.isSuccess()) << "Failed to get focus: " << get_result.error();
    EXPECT_DOUBLE_EQ(1.5, get_result.value());
}

TEST_F(WfovCameraTests, InvalidZoomValue) {
    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess()) << "Failed to connect: " << connect_result.error();

    const auto result = camera->setZoom(-1.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Invalid zoom level: Value must be greater than zero");
}
