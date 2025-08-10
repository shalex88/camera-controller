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
    ControllerFactoryTests() {
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "");
        core_ = std::make_unique<CoreMock>();
    }

    std::string server_address = "50051";
    std::shared_ptr<LayerLogger> logger_impl_;
    std::unique_ptr<core::ICore> core_;
};

TEST_F(ControllerFactoryTests, CreateGrpcServiceSuccess) {
    const auto service = api::ApiControllerFactory::createController("grpc", server_address, std::move(core_), logger_impl_);
    ASSERT_NE(nullptr, service);
    ASSERT_TRUE(service.get() != nullptr);
}

TEST_F(ControllerFactoryTests, ThrowsOnUnknownType) {
    EXPECT_THROW(
        api::ApiControllerFactory::createController("unknown", server_address, std::move(core_), logger_impl_),
        std::invalid_argument
    );
}
