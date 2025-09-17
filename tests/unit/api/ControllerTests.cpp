#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/ApiController.h"
#include "api/RequestHandler.h"
#include "api/ITransport.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class TransportMock: public api::ITransport {
public:
    MOCK_METHOD(Result<void>, start, (const std::string&), (override));
    MOCK_METHOD(Result<void>, stop, (), (override));
    MOCK_METHOD(Result<void>, runLoop, (), (override));
};

class RequestHandlerMock: public api::IRequestHandler {
public:
    MOCK_METHOD(Result<void>, start, (), (override));
    MOCK_METHOD(Result<void>, stop, (), (override));
    MOCK_METHOD(bool, isRunning, (), (const, override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
};

class ControllerTests : public Test {
protected:
    void SetUp() override {
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "API");
        request_handler = std::make_shared<NiceMock<RequestHandlerMock>>();
        transport = new NiceMock<TransportMock>();
        auto transport_obj = std::unique_ptr<api::ITransport>(transport);
        controller = std::make_unique<api::ApiController>(request_handler, std::move(transport_obj), server_address, logger_impl_);
    }

    std::shared_ptr<NiceMock<RequestHandlerMock>> request_handler;
    NiceMock<TransportMock>* transport {};
    std::unique_ptr<api::ApiController> controller;
    std::string server_address = "50051";
    std::shared_ptr<LayerLogger> logger_impl_;
};

TEST_F(ControllerTests, CreationSuccess) {
    ASSERT_NE(nullptr, controller);
}

TEST_F(ControllerTests, CreationFailNoController) {
    EXPECT_THROW(api::ApiController controller(
        nullptr,
        std::make_unique<NiceMock<TransportMock>>(),
        server_address, logger_impl_), std::invalid_argument);
}

TEST_F(ControllerTests, CreationFailNoTransport) {
    EXPECT_THROW(api::ApiController controller(
        request_handler,
        nullptr,
        server_address, logger_impl_), std::invalid_argument);
}

TEST_F(ControllerTests, CreationFailEmptyPort) {
    EXPECT_THROW(api::ApiController controller(
        request_handler,
        std::make_unique<NiceMock<TransportMock>>(),
        "", logger_impl_), std::invalid_argument);
}

TEST_F(ControllerTests, StartStopSuccess) {
    const auto start_result = controller->startAsync();
    ASSERT_TRUE(start_result.isSuccess());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // stop() should be triggered by the destructor
    EXPECT_CALL(*transport, stop())
    .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*request_handler, stop())
        .WillOnce(Return(Result<void>::success()));
}

TEST_F(ControllerTests, StartFailOnRequestHandlerStartFail) {
    EXPECT_CALL(*request_handler, start())
        .WillOnce(Return(Result<void>::error("Request Handler start failed")));

    const auto result = controller->startAsync();
    ASSERT_TRUE(result.isError());
}

TEST_F(ControllerTests, StartFailOnTransportStartFail) {
    EXPECT_CALL(*transport, start(server_address))
        .WillOnce(Return(Result<void>::error("Transport start failed")));

    const auto result = controller->startAsync();
    ASSERT_TRUE(result.isError());
}

TEST_F(ControllerTests, StopSuccessIfNotRunning) {
    const auto result = controller->stop();
    ASSERT_TRUE(result.isSuccess()) << "Stop should succeed if not running";
}

TEST_F(ControllerTests, StopSuccessIfRunning) {
    const auto result = controller->stop();
    ASSERT_TRUE(result.isSuccess()) << "Stop should succeed if not running";
}

TEST_F(ControllerTests, StopFailsIfTransportStopFails) {
    const auto start_result = controller->startAsync();
    ASSERT_TRUE(start_result.isSuccess());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_CALL(*transport, stop())
        .WillOnce(Return(Result<void>::error("Transport stop failed")));

    const auto stop_result = controller->stop();
    ASSERT_TRUE(stop_result.isError());
}

TEST_F(ControllerTests, StopFailsIfRequestHandlerStopFails) {
    const auto start_result = controller->startAsync();
    ASSERT_TRUE(start_result.isSuccess());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_CALL(*request_handler, stop())
        .WillOnce(Return(Result<void>::error("Request Handler stop failed")));

    const auto stop_result = controller->stop();
    ASSERT_TRUE(stop_result.isError());
}
