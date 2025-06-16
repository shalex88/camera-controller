#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "data/NfovCamera.h"

using namespace camera_service;
using namespace testing;

class NfovCameraTests : public Test {
protected:
    data::NfovCamera camera;
};

TEST_F(NfovCameraTests, CanBeConstructed) {
    EXPECT_FALSE(camera.isConnected());
}

TEST_F(NfovCameraTests, InitiallyNotConnected) {
    EXPECT_FALSE(camera.isConnected());
}

TEST_F(NfovCameraTests, ConnectDisconnect) {
    EXPECT_TRUE(camera.connect());
    EXPECT_TRUE(camera.isConnected());

    camera.disconnect();
    EXPECT_FALSE(camera.isConnected());
}

TEST_F(NfovCameraTests, DisconnectWhenNotConnected) {
    EXPECT_NO_THROW(camera.disconnect());
}

TEST_F(NfovCameraTests, SetZoomWhenNotConnected) {
    EXPECT_THROW(camera.setZoom(2.0), data::CameraException);
}

TEST_F(NfovCameraTests, GetZoomWhenNotConnected) {
    EXPECT_THROW(camera.getZoom(), data::CameraException);
}

TEST_F(NfovCameraTests, SetFocusWhenNotConnected) {
    EXPECT_THROW(camera.setFocus(1.0), data::CameraException);
}

TEST_F(NfovCameraTests, GetFocusWhenNotConnected) {
    EXPECT_THROW(camera.getFocus(), data::CameraException);
}

TEST_F(NfovCameraTests, SetAndGetZoom) {
    camera.connect();

    camera.setZoom(2.0);
    EXPECT_DOUBLE_EQ(camera.getZoom(), 2.0);

    camera.setZoom(3.5);
    EXPECT_DOUBLE_EQ(camera.getZoom(), 3.5);
}

TEST_F(NfovCameraTests, SetInvalidZoom) {
    camera.connect();
    EXPECT_THROW(camera.setZoom(0.0), data::CameraException);
    EXPECT_THROW(camera.setZoom(-1.0), data::CameraException);
}

TEST_F(NfovCameraTests, SetAndGetFocus) {
    camera.connect();

    camera.setFocus(1.5);
    EXPECT_DOUBLE_EQ(camera.getFocus(), 1.5);

    camera.setFocus(-0.5);
    EXPECT_DOUBLE_EQ(camera.getFocus(), -0.5);
}
