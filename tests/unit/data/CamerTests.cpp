#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "data/NfovCamera.h"
#include "data/ICamera.h"
#include "common/types/Result.h"
#include "data/Camera.h"

using namespace camera_service;
using namespace testing;

class MockCameraStrategy final : public data::ICamera {
public:
    MOCK_METHOD(Result<void>, connect, (), (override));
    MOCK_METHOD(Result<void>, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
};

class CameraTests : public Test {
protected:
    void SetUp() override {
        auto camera_strategy_obj = std::make_unique<MockCameraStrategy>();
        camera_strategy = camera_strategy_obj.get();
        camera = std::make_unique<data::Camera>(std::move(camera_strategy_obj));
    }

    MockCameraStrategy* camera_strategy {};
    std::unique_ptr<data::ICamera> camera;
};

TEST_F(CameraTests, CanBeConstructed) {
    const auto camera = std::make_unique<data::NfovCamera>();
    ASSERT_NE(nullptr, camera);
}

TEST_F(CameraTests, InitiallyNotConnected) {
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(CameraTests, ConnectDisconnect) {
    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());
    EXPECT_TRUE(camera->isConnected());

    const auto disconnect_result = camera->disconnect();
    EXPECT_TRUE(disconnect_result.isSuccess());
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(CameraTests, ConnectWhenConnectedFails) {
    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess());

    const auto second_connect_result = camera->connect();
    EXPECT_TRUE(second_connect_result.isError());
    EXPECT_TRUE(second_connect_result.error().find("connected") != std::string::npos);
}

TEST_F(CameraTests, DisconnectWhenNotConnected) {
    const auto result = camera->disconnect();
    EXPECT_TRUE(result.isError());
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(CameraTests, SetZoomWhenNotConnected) {
    const auto result = camera->setZoom(2.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot set zoom: NFOV Camera not connected");
}

TEST_F(CameraTests, GetZoomWhenNotConnected) {
    const auto result = camera->getZoom();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot get zoom: NFOV Camera not connected");
}

TEST_F(CameraTests, SetFocusWhenNotConnected) {
    const auto result = camera->setFocus(1.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot set focus: NFOV Camera not connected");
}

TEST_F(CameraTests, GetFocusWhenNotConnected) {
    const auto result = camera->getFocus();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Cannot get focus: NFOV Camera not connected");
}

TEST_F(CameraTests, ZoomOperations) {
    constexpr auto expected_value = 2.0;

    EXPECT_CALL(*camera_strategy, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_strategy, setZoom(expected_value))
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_strategy, getZoom())
        .WillOnce(Return(Result<types::zoom>::success(expected_value)));

    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->setZoom(expected_value);
    EXPECT_TRUE(set_result.isSuccess());

    const auto get_result = camera->getZoom();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_DOUBLE_EQ(expected_value, get_result.value());
}

TEST_F(CameraTests, FocusOperations) {
    constexpr auto expected_value = 2.0;

    EXPECT_CALL(*camera_strategy, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_strategy, setFocus(expected_value))
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_strategy, getFocus())
        .WillOnce(Return(Result<types::focus>::success(expected_value)));

    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->setFocus(expected_value);
    EXPECT_TRUE(set_result.isSuccess());

    const auto get_result = camera->getFocus();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_DOUBLE_EQ(expected_value, get_result.value());
}

TEST_F(CameraTests, InvalidZoomValue) {
    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess());

    const auto result = camera->setZoom(-1.0);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Invalid zoom level: Value must be greater than zero");
}
