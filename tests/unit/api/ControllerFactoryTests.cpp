#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/ApiControllerFactory.h"
#include "common/Config/ConfigManager.h"
#include "api/ApiController.h"
#include "core/ICore.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class CoreMock final: public core::ICore {
public:
    MOCK_METHOD(Result<void>, initialize, (), (override));
    MOCK_METHOD(Result<void>, shutdown, (), (override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
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
    ApiConfig config;
    config.api = "grpc";  // Valid API type
    config.server_address = "localhost:50051";  // Valid server address format

    const auto service = api::ApiControllerFactory::createController(std::move(core_), logger_impl_, config);
    ASSERT_NE(nullptr, service);
    ASSERT_TRUE(service.get() != nullptr);
}

TEST_F(ControllerFactoryTests, ThrowsOnUnknownType) {
    ApiConfig config;
    config.api = "invalid_api";  // Invalid API type to trigger exception
    config.server_address = "localhost:50051";  // Valid server address format

    EXPECT_THROW(
        api::ApiControllerFactory::createController(std::move(core_), logger_impl_, config),
        std::invalid_argument
    );
}
