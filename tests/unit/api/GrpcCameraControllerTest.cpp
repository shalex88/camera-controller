#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "api/GrpcCameraController.h"

#include <memory>

#include "core/ICore.h"

using namespace camera_service::api;
using namespace camera_service::core;
using ::testing::Return;
using ::testing::Throw;
using ::testing::_;

class MockCore : public ICore {
public:
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));
    MOCK_METHOD(void, setZoom, (double), (override));
    MOCK_METHOD(double, getZoom, (), (const, override));
    MOCK_METHOD(void, setFocus, (double), (override));
    MOCK_METHOD(double, getFocus, (), (const, override));
};

TEST(GrpcCameraControllerTest, CreationSuccess) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_NO_THROW(GrpcCameraController controller(std::move(mockCore)));
}

TEST(GrpcCameraControllerTest, CreationFail) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_THROW(GrpcCameraController controller(nullptr), ControllerException);
}

TEST(GrpcCameraControllerTest, StartSuccess) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    GrpcCameraController controller(std::move(mockCore));
    EXPECT_TRUE(controller.start());
}

TEST(GrpcCameraControllerTest, StartCoreInitFails) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(false));
    GrpcCameraController controller(std::move(mockCore));
    EXPECT_THROW(controller.start(), ControllerException);
}

TEST(GrpcCameraControllerTest, StartCoreThrows) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Throw(CoreException("fail")));
    GrpcCameraController controller(std::move(mockCore));
    EXPECT_THROW(controller.start(), ControllerException);
}

TEST(GrpcCameraControllerTest, StopSuccessIfNotRunning) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, shutdown()).Times(0);
    GrpcCameraController controller(std::move(mockCore));
    EXPECT_NO_THROW(controller.stop());
}

TEST(GrpcCameraControllerTest, StopCallsShutdownSuccess) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, shutdown()).Times(1);
    GrpcCameraController controller(std::move(mockCore));
    controller.start();
    controller.stop();
}

TEST(GrpcCameraControllerTest, StopCallsShutdownFail) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, shutdown()).WillOnce(Throw(CoreException("fail")));
    GrpcCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_NO_THROW(controller.stop());
}

TEST(GrpcCameraControllerTest, SetZoomAndGetZoom) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setZoom(2.5)).Times(1);
    EXPECT_CALL(*mockCore, getZoom()).WillOnce(Return(2.5));
    GrpcCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_TRUE(controller.setZoom(2.5));
    EXPECT_DOUBLE_EQ(controller.getZoom(), 2.5);
}

TEST(GrpcCameraControllerTest, SetFocusAndGetFocus) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setFocus(1.1)).Times(1);
    EXPECT_CALL(*mockCore, getFocus()).WillOnce(Return(1.1));
    GrpcCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_TRUE(controller.setFocus(1.1));
    EXPECT_DOUBLE_EQ(controller.getFocus(), 1.1);
}

TEST(GrpcCameraControllerTest, ThrowsIfNotRunning) {
    auto mockCore = std::make_unique<MockCore>();
    GrpcCameraController controller(std::move(mockCore));
    EXPECT_THROW(controller.setZoom(1.0), ControllerException);
    EXPECT_THROW(controller.getZoom(), ControllerException);
    EXPECT_THROW(controller.setFocus(1.0), ControllerException);
    EXPECT_THROW(controller.getFocus(), ControllerException);
}

TEST(GrpcCameraControllerTest, SetZoomThrowsCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setZoom(_)).WillOnce(Throw(CoreException("fail")));
    GrpcCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.setZoom(1.0), ControllerException);
}

TEST(GrpcCameraControllerTest, GetZoomThrowsCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, getZoom()).WillOnce(Throw(CoreException("fail")));
    GrpcCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.getZoom(), ControllerException);
}


TEST(GrpcCameraControllerTest, SetFocusThrowsCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setFocus(_)).WillOnce(Throw(CoreException("fail")));
    GrpcCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.setFocus(1.0), ControllerException);
}

TEST(GrpcCameraControllerTest, GetZoomFocusCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, getFocus()).WillOnce(Throw(CoreException("fail")));
    GrpcCameraController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.getFocus(), ControllerException);
}