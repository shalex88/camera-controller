#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "api/ControllerFactory.h"
#include "api/Controller.h"
#include "core/ICore.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class MockCore final: public core::ICore {
public:
    MOCK_METHOD(Result<void>, initialize, (), (override));
    MOCK_METHOD(Result<void>, shutdown, (), (override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
};

class ControllerFactoryTests : public Test {
protected:
    static std::unique_ptr<MockCore> createMockCore() {
        return std::make_unique<MockCore>();
    }
    std::string port = "50051";
};

TEST_F(ControllerFactoryTests, CreateGrpcControllerSuccess) {
    const auto controller = api::ControllerFactory::createController("grpc", port, createMockCore());
    ASSERT_NE(nullptr, controller);
    EXPECT_TRUE(controller.get() != nullptr);
}

TEST_F(ControllerFactoryTests, ThrowsOnUnknownType) {
    EXPECT_THROW(
        api::ControllerFactory::createController("unknown", port, createMockCore()),
        std::invalid_argument
    );
}
