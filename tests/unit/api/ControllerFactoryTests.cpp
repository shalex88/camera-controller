#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "api/ControllerFactory.h"

#include "api/ApiController.h"
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

class ControllerFactoryTests : public Test {
protected:
    static std::unique_ptr<MockCore> createMockCore() {
        return std::make_unique<MockCore>();
    }
    std::string port = "50051";
};

TEST_F(ControllerFactoryTests, CreateGrpcControllerSuccess) {
    auto controller = api::ControllerFactory::createController("grpc", port, createMockCore());
    ASSERT_NE(nullptr, controller);
    EXPECT_TRUE(dynamic_cast<api::ApiController*>(controller.get()) != nullptr);
}

TEST_F(ControllerFactoryTests, ThrowsOnUnknownType) {
    EXPECT_THROW(
        api::ControllerFactory::createController("unknown", port, createMockCore()),
        api::ControllerException
    );
}
