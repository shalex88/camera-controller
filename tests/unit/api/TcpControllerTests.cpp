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

TEST(TcpControllerTests, CreationSuccess) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_NO_THROW(api::TcpController controller(std::move(mock_core)));
}

TEST(TcpControllerTests, CreationFail) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_THROW(api::TcpController controller(nullptr), api::ControllerException );
}

TEST(TcpControllerTests, StartSuccess) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    api::TcpController controller(std::move(mock_core));
    EXPECT_TRUE(controller.start());
}

TEST(TcpControllerTests, StartCoreInitFails) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(false));
    api::TcpController controller(std::move(mock_core));
    EXPECT_THROW(controller.start(), api::ControllerException );
}

TEST(TcpControllerTests, StartCoreThrows) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mock_core));
    EXPECT_THROW(controller.start(), api::ControllerException );
}

TEST(TcpControllerTests, StopSuccessIfNotRunning) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, shutdown()).Times(0);
    api::TcpController controller(std::move(mock_core));
    EXPECT_NO_THROW(controller.stop());
}

TEST(TcpControllerTests, StopCallsShutdownSuccess) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, shutdown()).Times(1);
    api::TcpController controller(std::move(mock_core));
    controller.start();
    controller.stop();
}

TEST(TcpControllerTests, StopCallsShutdownFail) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, shutdown()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mock_core));
    controller.start();
    EXPECT_NO_THROW(controller.stop());
}

TEST(TcpControllerTests, SetZoomAndGetZoom) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, setZoom(2.5)).Times(1);
    EXPECT_CALL(*mock_core, getZoom()).WillOnce(Return(2.5));
    api::TcpController controller(std::move(mock_core));
    controller.start();
    EXPECT_TRUE(controller.setZoom(2.5));
    EXPECT_DOUBLE_EQ(controller.getZoom(), 2.5);
}

TEST(TcpControllerTests, SetFocusAndGetFocus) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, setFocus(1.1)).Times(1);
    EXPECT_CALL(*mock_core, getFocus()).WillOnce(Return(1.1));
    api::TcpController controller(std::move(mock_core));
    controller.start();
    EXPECT_TRUE(controller.setFocus(1.1));
    EXPECT_DOUBLE_EQ(controller.getFocus(), 1.1);
}

TEST(TcpControllerTests, ThrowsIfNotRunning) {
    auto mock_core = std::make_unique<MockCore>();
    api::TcpController controller(std::move(mock_core));
    EXPECT_THROW(controller.setZoom(1.0), api::ControllerException );
    EXPECT_THROW(controller.getZoom(), api::ControllerException );
    EXPECT_THROW(controller.setFocus(1.0), api::ControllerException );
    EXPECT_THROW(controller.getFocus(), api::ControllerException );
}

TEST(TcpControllerTests, SetZoomThrowsCoreException) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, setZoom(_)).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mock_core));
    controller.start();
    EXPECT_THROW(controller.setZoom(1.0), api::ControllerException );
}

TEST(TcpControllerTests, GetZoomThrowsCoreException) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, getZoom()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mock_core));
    controller.start();
    EXPECT_THROW(controller.getZoom(), api::ControllerException );
}


TEST(TcpControllerTests, SetFocusThrowsCoreException) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, setFocus(_)).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mock_core));
    controller.start();
    EXPECT_THROW(controller.setFocus(1.0), api::ControllerException );
}

TEST(TcpControllerTests, GetZoomFocusCoreException) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, getFocus()).WillOnce(Throw(core::CoreException("fail")));
    api::TcpController controller(std::move(mock_core));
    controller.start();
    EXPECT_THROW(controller.getFocus(), api::ControllerException );
}