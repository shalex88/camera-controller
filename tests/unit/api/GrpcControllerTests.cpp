#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "api/GrpcController.h"

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

TEST(GrpcControllerTests, CreationSuccess) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_NO_THROW(api::GrpcController controller(std::move(mock_core)));
}

TEST(GrpcControllerTests, CreationFail) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_THROW(api::GrpcController controller(nullptr), api::ControllerException );
}

TEST(GrpcControllerTests, StartSuccess) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    api::GrpcController controller(std::move(mock_core));
    EXPECT_TRUE(controller.start());
}

TEST(GrpcControllerTests, StartCoreInitFails) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(false));
    api::GrpcController controller(std::move(mock_core));
    EXPECT_THROW(controller.start(), api::ControllerException );
}

TEST(GrpcControllerTests, StartCoreThrows) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Throw(core::CoreException("fail")));
    api::GrpcController controller(std::move(mock_core));
    EXPECT_THROW(controller.start(), api::ControllerException );
}

TEST(GrpcControllerTests, StopSuccessIfNotRunning) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, shutdown()).Times(0);
    api::GrpcController controller(std::move(mock_core));
    EXPECT_NO_THROW(controller.stop());
}

TEST(GrpcControllerTests, StopCallsShutdownSuccess) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, shutdown()).Times(1);
    api::GrpcController controller(std::move(mock_core));
    controller.start();
    controller.stop();
}

TEST(GrpcControllerTests, StopCallsShutdownFail) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, shutdown()).WillOnce(Throw(core::CoreException("fail")));
    api::GrpcController controller(std::move(mock_core));
    controller.start();
    EXPECT_NO_THROW(controller.stop());
}

TEST(GrpcControllerTests, SetZoomAndGetZoom) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, setZoom(2.5)).Times(1);
    EXPECT_CALL(*mock_core, getZoom()).WillOnce(Return(2.5));
    api::GrpcController controller(std::move(mock_core));
    controller.start();
    EXPECT_TRUE(controller.setZoom(2.5));
    EXPECT_DOUBLE_EQ(controller.getZoom(), 2.5);
}

TEST(GrpcControllerTests, SetFocusAndGetFocus) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, setFocus(1.1)).Times(1);
    EXPECT_CALL(*mock_core, getFocus()).WillOnce(Return(1.1));
    api::GrpcController controller(std::move(mock_core));
    controller.start();
    EXPECT_TRUE(controller.setFocus(1.1));
    EXPECT_DOUBLE_EQ(controller.getFocus(), 1.1);
}

TEST(GrpcControllerTests, ThrowsIfNotRunning) {
    auto mock_core = std::make_unique<MockCore>();
    api::GrpcController controller(std::move(mock_core));
    EXPECT_THROW(controller.setZoom(1.0), api::ControllerException );
    EXPECT_THROW(controller.getZoom(), api::ControllerException );
    EXPECT_THROW(controller.setFocus(1.0), api::ControllerException );
    EXPECT_THROW(controller.getFocus(), api::ControllerException );
}

TEST(GrpcControllerTests, SetZoomThrowsCoreException) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, setZoom(_)).WillOnce(Throw(core::CoreException("fail")));
    api::GrpcController controller(std::move(mock_core));
    controller.start();
    EXPECT_THROW(controller.setZoom(1.0), api::ControllerException );
}

TEST(GrpcControllerTests, GetZoomThrowsCoreException) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, getZoom()).WillOnce(Throw(core::CoreException("fail")));
    api::GrpcController controller(std::move(mock_core));
    controller.start();
    EXPECT_THROW(controller.getZoom(), api::ControllerException );
}


TEST(GrpcControllerTests, SetFocusThrowsCoreException) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, setFocus(_)).WillOnce(Throw(core::CoreException("fail")));
    api::GrpcController controller(std::move(mock_core));
    controller.start();
    EXPECT_THROW(controller.setFocus(1.0), api::ControllerException );
}

TEST(GrpcControllerTests, GetZoomFocusCoreException) {
    auto mock_core = std::make_unique<MockCore>();
    EXPECT_CALL(*mock_core, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*mock_core, getFocus()).WillOnce(Throw(core::CoreException("fail")));
    api::GrpcController controller(std::move(mock_core));
    controller.start();
    EXPECT_THROW(controller.getFocus(), api::ControllerException );
}