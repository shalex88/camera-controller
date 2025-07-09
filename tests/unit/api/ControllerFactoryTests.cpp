#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/ApiControllerFactory.h"
#include "api/ApiController.h"
#include "core/ICore.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class CoreMock final: public core::ICore {
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
    static std::unique_ptr<CoreMock> createMockCore() {
        return std::make_unique<CoreMock>();
    }
    std::string server_address = "50051";
};

TEST_F(ControllerFactoryTests, CreateGrpcServiceSuccess) {
    const auto service = api::ApiControllerFactory::createController("grpc", server_address, createMockCore());
    ASSERT_NE(nullptr, service);
    ASSERT_TRUE(service.get() != nullptr);
}

TEST_F(ControllerFactoryTests, ThrowsOnUnknownType) {
    EXPECT_THROW(
        api::ApiControllerFactory::createController("unknown", server_address, createMockCore()),
        std::invalid_argument
    );
}
