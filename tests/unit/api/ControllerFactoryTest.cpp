#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "api/ControllerFactory.h"

#include "api/GrpcController.h"
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

class ControllerFactoryTest : public Test {
protected:
    std::unique_ptr<MockCore> createMockCore() {
        return std::make_unique<MockCore>();
    }
};

TEST_F(ControllerFactoryTest, CreateGrpcControllerSuccess) {
    auto controller = api::ControllerFactory::createController("grpc", createMockCore());
    ASSERT_NE(nullptr, controller);
    EXPECT_TRUE(dynamic_cast<api::GrpcController*>(controller.get()) != nullptr);
}

TEST_F(ControllerFactoryTest, CreateTcpControllerSuccess) {
    auto controller = api::ControllerFactory::createController("tcp", createMockCore());
    ASSERT_NE(nullptr, controller);
    EXPECT_TRUE(dynamic_cast<api::TcpController*>(controller.get()) != nullptr);
}

TEST_F(ControllerFactoryTest, ThrowsOnUnknownType) {
    EXPECT_THROW(
        api::ControllerFactory::createController("unknown", createMockCore()),
        api::ControllerException
    );
}
