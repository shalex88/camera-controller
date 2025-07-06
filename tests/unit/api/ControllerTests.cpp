#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/Controller.h"
#include "api/RequestHandler.h"
#include "api/ITransport.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class TransportMock final: public api::ITransport {
public:
    MOCK_METHOD(Result<void>, start, (const std::string&), (override));
    MOCK_METHOD(Result<void>, stop, (), (override));
    MOCK_METHOD(Result<void>, runLoop, (), (override));
};

class RequestHandlerMock final: public api::IRequestHandler, api::ICameraOperations {
public:
    MOCK_METHOD(Result<void>, start, (), (override));
    MOCK_METHOD(Result<void>, stop, (), (override));
    MOCK_METHOD(bool, isRunning, (), (const, override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
};

class ControllerTests : public Test {
protected:
    void SetUp() override {
        request_handler = std::make_shared<RequestHandlerMock>();
        transport = new TransportMock();
        auto transport_obj = std::unique_ptr<api::ITransport>(transport);
        controller = std::make_unique<api::Controller>(request_handler, std::move(transport_obj), server_address);
    }

    std::shared_ptr<RequestHandlerMock> request_handler;
    TransportMock* transport {};
    std::unique_ptr<api::Controller> controller;
    std::string server_address = "50051";
};

TEST_F(ControllerTests, CreationSuccess) {
    ASSERT_NE(nullptr, controller);
}

TEST_F(ControllerTests, CreationFailNoController) {
    EXPECT_THROW(api::Controller controller(
        nullptr,
        std::make_unique<TransportMock>(),
        server_address), std::invalid_argument);
}

TEST_F(ControllerTests, CreationFailNoTransport) {
    EXPECT_THROW(api::Controller controller(
        request_handler,
        nullptr,
        server_address), std::invalid_argument);
}

TEST_F(ControllerTests, CreationFailEmptyPort) {
    EXPECT_THROW(api::Controller controller(
        request_handler,
        std::make_unique<TransportMock>(),
        ""), std::invalid_argument);
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

