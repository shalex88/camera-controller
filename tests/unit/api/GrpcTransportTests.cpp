#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/RequestHandler.h"
#include "api/GrpcTransport.h"
#include "core/ICore.h"
#include "common/types/Result.h"
#include "../../utils/GrpcClient.h"

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

class GrpcTransportTests : public Test {
protected:
    void SetUp() override {
        request_handler = std::make_shared<api::RequestHandler>(std::make_unique<CoreMock>());
        grpc_transport = std::make_unique<api::GrpcTransport>(request_handler);
    }

    std::shared_ptr<api::RequestHandler> request_handler;
    std::unique_ptr<api::GrpcTransport> grpc_transport;
    std::string server_address = "0.0.0.0:50051";
};

TEST_F(GrpcTransportTests, CreationSuccess) {
    ASSERT_NE(nullptr, grpc_transport);
}

TEST_F(GrpcTransportTests, CreationFailIfNoRequestHandler) {
    EXPECT_THROW(api::GrpcTransport transport(nullptr), std::invalid_argument);
}

TEST_F(GrpcTransportTests, StartServerSuccess) {
    const auto result = grpc_transport->start(server_address);
    ASSERT_TRUE(result.isSuccess());
    grpc_transport->stop();
}

TEST_F(GrpcTransportTests, StartServerOnInvalidPortShouldFail) {
    const auto result = grpc_transport->start("invalid_port");
    ASSERT_TRUE(result.isError());
}

TEST_F(GrpcTransportTests, StopServerWhenNotStartedShouldSucceed) {
    const auto result = grpc_transport->stop();
    ASSERT_TRUE(result.isSuccess());
}

TEST_F(GrpcTransportTests, StopRunningServerShouldSucceed) {
    // Start server first
    const auto start_result = grpc_transport->start(server_address);
    ASSERT_TRUE(start_result.isSuccess());

    // Then stop it
    const auto stop_result = grpc_transport->stop();
    ASSERT_TRUE(stop_result.isSuccess());
}

TEST_F(GrpcTransportTests, StopServerMultipleTimesShouldSucceed) {
    // Start and stop once
    grpc_transport->start(server_address);
    const auto first_stop = grpc_transport->stop();
    ASSERT_TRUE(first_stop.isSuccess());

    // Stop again when already stopped
    const auto second_stop = grpc_transport->stop();
    ASSERT_TRUE(second_stop.isSuccess());
}

TEST_F(GrpcTransportTests, RunLoopWithoutStartShouldFail) {
    const auto result = grpc_transport->runLoop();
    ASSERT_TRUE(result.isError());
}

TEST_F(GrpcTransportTests, RunLoopAfterStopShouldFail) {
    // Start and stop the server
    grpc_transport->start(server_address);
    grpc_transport->stop();

    // Try to run loop after stop
    const auto result = grpc_transport->runLoop();
    ASSERT_TRUE(result.isError());
}

TEST_F(GrpcTransportTests, RunLoopWithRunningServerShouldSucceed) {
    grpc_transport->start(server_address);

    std::thread server_thread([&] {
        const auto result = grpc_transport->runLoop();
        ASSERT_TRUE(result.isSuccess());
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    grpc_transport->stop();
    server_thread.join();
}