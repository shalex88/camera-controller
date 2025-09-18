#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "data/camera/AdimecCamera.h"
#include "data/ICameraHal.h"
#include "data/ICameraHw.h"
#include "common/types/Result.h"
#include "data/CameraHal.h"
#include "data/hw_interface/mmio/IRegisterImpl.h"
#include "data/hw_interface/mmio/RegistersMapManager.h"

using namespace camera_service;
using namespace testing;

class MockCameraHw : public data::ICameraHw {
public:
    MOCK_METHOD(Result<void>, connect, (), (override));
    MOCK_METHOD(Result<void>, disconnect, (), (override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(types::CameraLimits, getLimits, (), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
    types::CameraLimits limits {
        .min_zoom = 0,
        .max_zoom = 100,
        .min_focus = 0,
        .max_focus = 100,
    };
};

class MockRegisterImpl : public data::IRegisterImpl {
public:
    MOCK_METHOD(Result<void>, set, (uint32_t address, uint32_t value), (override));
    MOCK_METHOD(Result<uint32_t>, get, (uint32_t address), (const, override));
};

class CameraTests : public Test {
protected:
    CameraTests() {
        auto camera_strategy_obj = std::make_unique<NiceMock<MockCameraHw>>();
        camera_hw_ = camera_strategy_obj.get();
        EXPECT_CALL(*camera_hw_, getLimits())
            .WillOnce(Return(camera_hw_->limits));
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "Data");
        camera = std::make_unique<data::CameraHal>(std::move(camera_strategy_obj), logger_impl_);
    }

    NiceMock<MockCameraHw>* camera_hw_ {};
    std::unique_ptr<data::ICameraHal> camera;
    std::shared_ptr<LayerLogger> logger_impl_;
};

TEST_F(CameraTests, CanBeConstructed) {
    auto register_impl = std::make_unique<NiceMock<MockRegisterImpl>>();
    auto fpga_manager = std::make_unique<data::RegistersMapManager>(std::move(register_impl));
    const auto camera = std::make_unique<data::AdimecCamera>(std::move(fpga_manager));
    ASSERT_NE(nullptr, camera);
}

TEST_F(CameraTests, InitiallyNotConnected) {
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(CameraTests, ConnectDisconnect) {
    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess());
    ASSERT_TRUE(camera->isConnected());

    const auto disconnect_result = camera->disconnect();
    ASSERT_TRUE(disconnect_result.isSuccess());
    EXPECT_FALSE(camera->isConnected());
}

TEST_F(CameraTests, ConnectFailsWhenCameraCantConnect) {
    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::error("error")));

    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isError());
    ASSERT_FALSE(camera->isConnected());
}

TEST_F(CameraTests, ReconnectWhenAlreadyConnectedFails) {
    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess());

    const auto second_connect_result = camera->connect();
    ASSERT_TRUE(second_connect_result.isError());
    ASSERT_TRUE(second_connect_result.error().find("connected") != std::string::npos);
}

TEST_F(CameraTests, DisconnectWhenNotConnectedFails) {
    const auto result = camera->disconnect();
    ASSERT_TRUE(result.isError());
    EXPECT_FALSE(camera->isConnected());
}


TEST_F(CameraTests, DisonnectWhenCameraCantDisconnectFails) {
    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, disconnect())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(Result<void>::error("error")));

    EXPECT_TRUE(camera->connect().isSuccess());

    const auto connect_result = camera->disconnect();
    ASSERT_TRUE(connect_result.isError());
    ASSERT_TRUE(camera->isConnected());
}

TEST_F(CameraTests, SetZoomWhenNotConnectedFail) {
    const auto result = camera->setZoom(2);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Camera not connected");
}

TEST_F(CameraTests, GetZoomWhenNotConnectedFail) {
    const auto result = camera->getZoom();
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Camera not connected");
}

TEST_F(CameraTests, SetFocusWhenNotConnectedFail) {
    const auto result = camera->setFocus(1);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Camera not connected");
}

TEST_F(CameraTests, GetFocusWhenNotConnectedFail) {
    const auto result = camera->getFocus();
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Camera not connected");
}

TEST_F(CameraTests, SetValidZoomSuccess) {
    constexpr auto expected_value = 2;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, setZoom(expected_value))
        .WillOnce(Return(Result<void>::success()));

    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->setZoom(expected_value);
    ASSERT_TRUE(set_result.isSuccess());
}

TEST_F(CameraTests, SetInvalidZoomFail) {
    constexpr auto expected_value = -2;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));

    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->setZoom(expected_value);
    ASSERT_TRUE(set_result.isError());
}

TEST_F(CameraTests, SetValidZoomWhenCameraErrorFails) {
    constexpr auto expected_value = 2;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, setZoom(expected_value))
        .WillOnce(Return(Result<void>::error("error")));

    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->setZoom(expected_value);
    ASSERT_TRUE(set_result.isError());
}

TEST_F(CameraTests, GetValidZoomWhenCameraErrorFails) {
    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, getZoom())
        .WillOnce(Return(Result<types::zoom>::error("error")));

    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->getZoom();
    ASSERT_TRUE(set_result.isError());
}

TEST_F(CameraTests, GetValidZoomSuccess) {
    constexpr auto expected_value = 2u;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, getZoom())
        .WillOnce(Return(Result<types::zoom>::success(expected_value)));

    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->getZoom();
    EXPECT_TRUE(set_result.isSuccess());
    ASSERT_EQ(set_result.value<types::zoom>(), expected_value);
}

TEST_F(CameraTests, GetInvalidZoomFail) {
    constexpr auto expected_value = -2u;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, getZoom())
        .WillOnce(Return(Result<types::zoom>::success(expected_value)));

    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->getZoom();
    ASSERT_TRUE(set_result.isError());
}

TEST_F(CameraTests, SetValidFocusSuccess) {
    constexpr auto expected_value = 2u;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, setFocus(expected_value))
        .WillOnce(Return(Result<void>::success()));

    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->setFocus(expected_value);
    ASSERT_TRUE(set_result.isSuccess());
}

TEST_F(CameraTests, GetValidFocusSuccess) {
    constexpr auto expected_value = 2u;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, getFocus())
        .WillOnce(Return(Result<types::focus>::success(expected_value)));

    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->getFocus();
    EXPECT_TRUE(set_result.isSuccess());
    ASSERT_EQ(set_result.value<types::focus>(), expected_value);
}

TEST_F(CameraTests, SetValidFocusWhenCameraErrorFails) {
    constexpr auto expected_value = 2u;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, setFocus(expected_value))
        .WillOnce(Return(Result<void>::error("error")));

    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->setFocus(expected_value);
    ASSERT_TRUE(set_result.isError());
}

TEST_F(CameraTests, GetValidFocusWhenCameraErrorFails) {
    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, getFocus())
        .WillOnce(Return(Result<types::focus>::error("error")));

    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->getFocus();
    ASSERT_TRUE(set_result.isError());
}

TEST_F(CameraTests, SetInvalidFocusFail) {
    constexpr auto expected_value = -2u;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));

    const auto connect_result = camera->connect();
    ASSERT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->setFocus(expected_value);
    ASSERT_TRUE(set_result.isError());
}


TEST_F(CameraTests, GetInvalidFocusFail) {
    constexpr auto expected_value = -2u;

    EXPECT_CALL(*camera_hw_, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera_hw_, getFocus())
        .WillOnce(Return(Result<types::focus>::success(expected_value)));

    const auto connect_result = camera->connect();
    EXPECT_TRUE(connect_result.isSuccess());

    const auto set_result = camera->getFocus();
    ASSERT_TRUE(set_result.isError());
}
