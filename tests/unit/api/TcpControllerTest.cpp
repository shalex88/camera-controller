#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "api/TcpController.h"

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

TEST(TcpControllerTest, CreationSuccess) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_NO_THROW(api::TcpController controller(std::move(mockCore)));
}

TEST(TcpControllerTest, CreationFail) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_THROW(api::TcpController controller(nullptr), api::ControllerException );
}

TEST(TcpControllerTest, StartSuccess) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    api::TcpController controller(std::move(mockCore));
    EXPECT_TRUE(controller.start());
}

TEST(TcpControllerTest, StartCoreInitFails) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(false));
    api::TcpController controller(std::move(mockCore));
    EXPECT_THROW(controller.start(), api::ControllerException );
}

TEST(TcpControllerTest, StartCoreThrows) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mockCore));
    EXPECT_THROW(controller.start(), api::ControllerException );
}

TEST(TcpControllerTest, StopSuccessIfNotRunning) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, shutdown()).Times(0);
    api::TcpController controller(std::move(mockCore));
    EXPECT_NO_THROW(controller.stop());
}

TEST(TcpControllerTest, StopCallsShutdownSuccess) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, shutdown()).Times(1);
    api::TcpController controller(std::move(mockCore));
    controller.start();
    controller.stop();
}

TEST(TcpControllerTest, StopCallsShutdownFail) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, shutdown()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mockCore));
    controller.start();
    EXPECT_NO_THROW(controller.stop());
}

TEST(TcpControllerTest, SetZoomAndGetZoom) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setZoom(2.5)).Times(1);
    EXPECT_CALL(*mockCore, getZoom()).WillOnce(Return(2.5));
    api::TcpController controller(std::move(mockCore));
    controller.start();
    EXPECT_TRUE(controller.setZoom(2.5));
    EXPECT_DOUBLE_EQ(controller.getZoom(), 2.5);
}

TEST(TcpControllerTest, SetFocusAndGetFocus) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setFocus(1.1)).Times(1);
    EXPECT_CALL(*mockCore, getFocus()).WillOnce(Return(1.1));
    api::TcpController controller(std::move(mockCore));
    controller.start();
    EXPECT_TRUE(controller.setFocus(1.1));
    EXPECT_DOUBLE_EQ(controller.getFocus(), 1.1);
}

TEST(TcpControllerTest, ThrowsIfNotRunning) {
    auto mockCore = std::make_unique<MockCore>();
    api::TcpController controller(std::move(mockCore));
    EXPECT_THROW(controller.setZoom(1.0), api::ControllerException );
    EXPECT_THROW(controller.getZoom(), api::ControllerException );
    EXPECT_THROW(controller.setFocus(1.0), api::ControllerException );
    EXPECT_THROW(controller.getFocus(), api::ControllerException );
}

TEST(TcpControllerTest, SetZoomThrowsCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setZoom(_)).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.setZoom(1.0), api::ControllerException );
}

TEST(TcpControllerTest, GetZoomThrowsCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, getZoom()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.getZoom(), api::ControllerException );
}


TEST(TcpControllerTest, SetFocusThrowsCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, setFocus(_)).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.setFocus(1.0), api::ControllerException );
}

TEST(TcpControllerTest, GetZoomFocusCoreException) {
    auto mockCore = std::make_unique<MockCore>();
    EXPECT_CALL(*mockCore, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mockCore, getFocus()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mockCore));
    controller.start();
    EXPECT_THROW(controller.getFocus(), api::ControllerException );
}