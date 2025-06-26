#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/RequestHandler.h"
#include "api/GrpcTransport.h"
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

class GrpcTransportTests : public Test {
protected:
    void SetUp() override {
        request_handler = std::make_shared<api::RequestHandler>(std::make_unique<MockCore>());
    }

    std::shared_ptr<api::RequestHandler> request_handler;
    std::string port = "50051";
};

TEST_F(GrpcTransportTests, CreationSuccess) {
    ASSERT_NE(nullptr, std::make_unique<api::GrpcTransport>(request_handler));
}

TEST_F(GrpcTransportTests, CreationFailNoController) {
    EXPECT_THROW(api::GrpcTransport transport(nullptr), std::invalid_argument);
}