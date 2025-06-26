#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/Controller.h"
#include "api/RequestHandler.h"
#include "api/ITransport.h"
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

class MockTransport final: public api::ITransport {
public:
    MOCK_METHOD(Result<void>, start, (const std::string&), (override));
    MOCK_METHOD(Result<void>, stop, (), (override));
    MOCK_METHOD(Result<void>, runLoop, (), (override));
};

class ControllerTests : public Test {
protected:
    void SetUp() override {
        core = std::make_unique<MockCore>();
        controller = std::make_unique<api::RequestHandler>(std::move(core));
        transport = std::make_unique<MockTransport>();
    }

    std::unique_ptr<api::RequestHandler> controller;
    std::unique_ptr<MockTransport> transport;
    std::unique_ptr<MockCore> core;
    std::string port = "50051";
};

TEST_F(ControllerTests, CreationSuccess) {
    auto request_handler = std::make_unique<api::RequestHandler>(std::make_unique<MockCore>());
    auto transport_tmp = std::make_unique<MockTransport>();

    ASSERT_NE(nullptr, std::make_unique<api::Controller>(
        std::move(request_handler),
        std::move(transport_tmp),
        port));
}

TEST_F(ControllerTests, CreationFailNoController) {
    EXPECT_THROW(api::Controller controller(
        nullptr,
        std::make_unique<MockTransport>(),
        port), std::invalid_argument);
}

TEST_F(ControllerTests, CreationFailNoTransport) {
    EXPECT_THROW(api::Controller controller(
        std::make_unique<api::RequestHandler>(std::make_unique<MockCore>()),
        nullptr,
        port), std::invalid_argument);
}

TEST_F(ControllerTests, CreationFailEmptyPort) {
    EXPECT_THROW(api::Controller controller(
        std::make_unique<api::RequestHandler>(std::make_unique<MockCore>()),
        std::make_unique<MockTransport>(),
        ""), std::invalid_argument);
}

TEST_F(ControllerTests, StartStopSuccess) {
    // Recreate test fixtures for this test
    auto core_ptr = std::make_unique<MockCore>();
    auto* core_raw = core_ptr.get();
    auto request_handler = std::make_unique<api::RequestHandler>(std::move(core_ptr));
    auto transport = std::make_unique<MockTransport>();
    auto* transport_raw = transport.get();

    EXPECT_CALL(*core_raw, initialize())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*transport_raw, start(port))
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*transport_raw, runLoop())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*transport_raw, stop())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*core_raw, shutdown())
        .WillOnce(Return(Result<void>::success()));

    auto controller = std::make_unique<api::Controller>(
        std::move(request_handler),
        std::move(transport),
        port);

    const auto start_result = controller->startAsync();
    EXPECT_TRUE(start_result.isSuccess()) << "Failed to start: " << start_result.error();

    // Give the controller thread time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto stop_result = controller->stop();
    EXPECT_TRUE(stop_result.isSuccess()) << "Failed to stop: " << stop_result.error();
}

TEST_F(ControllerTests, StartFailOnControllerStart) {
    auto core_ptr = std::make_unique<MockCore>();
    auto* core_raw = core_ptr.get();
    auto request_handler = std::make_unique<api::RequestHandler>(std::move(core_ptr));
    auto transport = std::make_unique<MockTransport>();

    EXPECT_CALL(*core_raw, initialize())
        .WillOnce(Return(Result<void>::error("Controller start failed")));

    auto controller = std::make_unique<api::Controller>(
        std::move(request_handler),
        std::move(transport),
        port);

    const auto result = controller->startAsync();
    EXPECT_TRUE(result.isError());
    EXPECT_TRUE(result.error().find("Failed to start controller") != std::string::npos);
}

TEST_F(ControllerTests, StartFailOnTransportStart) {
    // Recreate test fixtures for this test
    auto core_ptr = std::make_unique<MockCore>();
    auto* core_raw = core_ptr.get();
    auto request_handler = std::make_unique<api::RequestHandler>(std::move(core_ptr));
    auto transport = std::make_unique<MockTransport>();
    auto* transport_raw = transport.get();

    EXPECT_CALL(*core_raw, initialize())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*transport_raw, start(port))
        .WillOnce(Return(Result<void>::error("Transport start failed")));
    EXPECT_CALL(*core_raw, shutdown())
        .WillOnce(Return(Result<void>::success()));

    auto controller = std::make_unique<api::Controller>(
        std::move(request_handler),
        std::move(transport),
        port);

    const auto result = controller->startAsync();
    EXPECT_TRUE(result.isError());
    EXPECT_TRUE(result.error().find("Failed to start transport") != std::string::npos);
}
