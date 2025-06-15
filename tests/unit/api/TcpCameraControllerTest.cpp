#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "api/TcpCameraController.h"

#include "core/ICore.h"

using namespace camera_service;
using namespace testing;

class MockCore final: public core::ICore {
public:
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));
    MOCK_METHOD(void, setZoom, (double), (override));
    MOCK_METHOD(double, getZoom, (), (const, override));
    MOCK_METHOD(void, setFocus, (double), (override));
    MOCK_METHOD(double, getFocus, (), (const, override));
};

TEST(TcpCameraControllerTest, CreationSuccess) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_NO_THROW(api::TcpCameraController controller(std::move(mockCore)));
}

TEST(TcpCameraControllerTest, CreationFail) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_THROW(api::TcpCameraController controller(nullptr), api::ControllerException );
}

TEST(TcpCameraControllerTest, StartSuccess) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    api::TcpCameraController controller(std::move(mockCore));
    EXPECT_TRUE(controller.start());
}

TEST(TcpCameraControllerTest, StartCoreInitFails) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(false));
    api::TcpCameraController controller(std::move(mockCore));
    EXPECT_THROW(controller.start(), api::ControllerException );
}

TEST(TcpCameraControllerTest, StartCoreThrows) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpCameraController controller(std::move(mockCore));
    EXPECT_THROW(controller.start(), api::ControllerException );
}

TEST(TcpCameraControllerTest, StopSuccessIfNotRunning) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, shutdown()).Times(0);
    api::TcpCameraController controller(std::move(mockCore));
    EXPECT_NO_THROW(controller.stop());
}

TEST(TcpCameraControllerTest, StopCallsShutdownSuccess) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, shutdown()).Times(1);
    api::TcpCameraController controller(std::move(mockCore));
    controller.start();
    controller.stop();
}

TEST(TcpCameraControllerTest, StopCallsShutdownFail) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, shutdown()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_NO_THROW(controller.stop());
}

TEST(TcpCameraControllerTest, SetZoomAndGetZoom) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setZoom(2.5)).Times(1);
    EXPECT_CALL(*mockCore, getZoom()).WillOnce(Return(2.5));
    api::TcpCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_TRUE(controller.setZoom(2.5));
    EXPECT_DOUBLE_EQ(controller.getZoom(), 2.5);
}

TEST(TcpCameraControllerTest, SetFocusAndGetFocus) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setFocus(1.1)).Times(1);
    EXPECT_CALL(*mockCore, getFocus()).WillOnce(Return(1.1));
    api::TcpCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_TRUE(controller.setFocus(1.1));
    EXPECT_DOUBLE_EQ(controller.getFocus(), 1.1);
}

TEST(TcpCameraControllerTest, ThrowsIfNotRunning) {
    auto mockCore = std::make_unique<MockCore>();
    api::TcpCameraController controller(std::move(mockCore));
    EXPECT_THROW(controller.setZoom(1.0), api::ControllerException );
    EXPECT_THROW(controller.getZoom(), api::ControllerException );
    EXPECT_THROW(controller.setFocus(1.0), api::ControllerException );
    EXPECT_THROW(controller.getFocus(), api::ControllerException );
}

TEST(TcpCameraControllerTest, SetZoomThrowsCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setZoom(_)).WillOnce(Throw(core::CoreException("fail")));
    api::TcpCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.setZoom(1.0), api::ControllerException );
}

TEST(TcpCameraControllerTest, GetZoomThrowsCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, getZoom()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.getZoom(), api::ControllerException );
}


TEST(TcpCameraControllerTest, SetFocusThrowsCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setFocus(_)).WillOnce(Throw(core::CoreException("fail")));
    api::TcpCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.setFocus(1.0), api::ControllerException );
}

TEST(TcpCameraControllerTest, GetZoomFocusCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, getFocus()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.getFocus(), api::ControllerException );
}