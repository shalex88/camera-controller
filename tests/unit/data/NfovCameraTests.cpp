#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "data/NfovCamera.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class NfovCameraTests : public Test {
protected:
    void SetUp() override {
        camera = std::make_unique<data::NfovCamera>();
    }

    std::unique_ptr<data::NfovCamera> camera;
};

TEST_F(NfovCameraTests, CanBeConstructed) {
    auto camera = std::make_unique<data::NfovCamera>();
    ASSERT_NE(nullptr, camera);
}

TEST_F(NfovCameraTests, InitiallyNotConnected) {
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(NfovCameraTests, ConnectDisconnect) {
    auto connectResult = camera->connect();
    EXPECT_TRUE(connectResult.isSuccess()) << "Failed to connect: " << connectResult.error();
    EXPECT_TRUE(camera->isConnected());

    auto disconnectResult = camera->disconnect();
    EXPECT_TRUE(disconnectResult.isSuccess()) << "Failed to disconnect: " << disconnectResult.error();
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(NfovCameraTests, DisconnectWhenNotConnected) {
    auto result = camera->disconnect();
    EXPECT_TRUE(result.isError());
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(NfovCameraTests, SetZoomWhenNotConnected) {
    auto result = camera->setZoom(2.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot set zoom: NFOV Camera not connected");
}

TEST_F(NfovCameraTests, GetZoomWhenNotConnected) {
    auto result = camera->getZoom();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot get zoom: NFOV Camera not connected");
}

TEST_F(NfovCameraTests, SetFocusWhenNotConnected) {
    auto result = camera->setFocus(1.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot set focus: NFOV Camera not connected");
}

TEST_F(NfovCameraTests, GetFocusWhenNotConnected) {
    auto result = camera->getFocus();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot get focus: NFOV Camera not connected");
}

TEST_F(NfovCameraTests, ZoomOperations) {
    auto connectResult = camera->connect();
    ASSERT_TRUE(connectResult.isSuccess()) << "Failed to connect: " << connectResult.error();

    auto setResult = camera->setZoom(2.0);
    EXPECT_TRUE(setResult.isSuccess()) << "Failed to set zoom: " << setResult.error();

    auto getResult = camera->getZoom();
    ASSERT_TRUE(getResult.isSuccess()) << "Failed to get zoom: " << getResult.error();
    EXPECT_DOUBLE_EQ(2.0, getResult.value());
}

TEST_F(NfovCameraTests, FocusOperations) {
    auto connectResult = camera->connect();
    ASSERT_TRUE(connectResult.isSuccess()) << "Failed to connect: " << connectResult.error();

    auto setResult = camera->setFocus(1.5);
    EXPECT_TRUE(setResult.isSuccess()) << "Failed to set focus: " << setResult.error();

    auto getResult = camera->getFocus();
    ASSERT_TRUE(getResult.isSuccess()) << "Failed to get focus: " << getResult.error();
    EXPECT_DOUBLE_EQ(1.5, getResult.value());
}

TEST_F(NfovCameraTests, InvalidZoomValue) {
    auto connectResult = camera->connect();
    ASSERT_TRUE(connectResult.isSuccess()) << "Failed to connect: " << connectResult.error();

    auto result = camera->setZoom(-1.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Invalid zoom level: Value must be greater than zero");
}
