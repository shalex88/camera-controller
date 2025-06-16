#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "data/WfovCamera.h"

using namespace camera_service;
using namespace testing;

class WfovCameraTests : public Test {
protected:
    data::WfovCamera camera;
};

TEST_F(WfovCameraTests, CanBeConstructed) {
    EXPECT_FALSE(camera.isConnected());
}

TEST_F(WfovCameraTests, InitiallyNotConnected) {
    EXPECT_FALSE(camera.isConnected());
}

TEST_F(WfovCameraTests, ConnectDisconnect) {
    EXPECT_TRUE(camera.connect());
    EXPECT_TRUE(camera.isConnected());

    camera.disconnect();
    EXPECT_FALSE(camera.isConnected());
}

TEST_F(WfovCameraTests, DisconnectWhenNotConnected) {
    EXPECT_NO_THROW(camera.disconnect());
}

TEST_F(WfovCameraTests, SetZoomWhenNotConnected) {
    EXPECT_THROW(camera.setZoom(2.0), data::CameraException);
}

TEST_F(WfovCameraTests, GetZoomWhenNotConnected) {
    EXPECT_THROW(camera.getZoom(), data::CameraException);
}

TEST_F(WfovCameraTests, SetFocusWhenNotConnected) {
    EXPECT_THROW(camera.setFocus(1.0), data::CameraException);
}

TEST_F(WfovCameraTests, GetFocusWhenNotConnected) {
    EXPECT_THROW(camera.getFocus(), data::CameraException);
}

TEST_F(WfovCameraTests, SetAndGetZoom) {
    camera.connect();

    camera.setZoom(2.0);
    EXPECT_DOUBLE_EQ(camera.getZoom(), 2.0);

    camera.setZoom(3.5);
    EXPECT_DOUBLE_EQ(camera.getZoom(), 3.5);
}

TEST_F(WfovCameraTests, SetInvalidZoom) {
    camera.connect();
    EXPECT_THROW(camera.setZoom(0.0), data::CameraException);
    EXPECT_THROW(camera.setZoom(-1.0), data::CameraException);
}

TEST_F(WfovCameraTests, SetAndGetFocus) {
    camera.connect();

    camera.setFocus(1.5);
    EXPECT_DOUBLE_EQ(camera.getFocus(), 1.5);

    camera.setFocus(-0.5);
    EXPECT_DOUBLE_EQ(camera.getFocus(), -0.5);
}
