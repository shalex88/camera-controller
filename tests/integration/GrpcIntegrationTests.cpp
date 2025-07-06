#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include <chrono>
#include <memory>
#include <thread>

#include "api/GrpcTransport.h"
#include "../../utils/GrpcClient.h"

using namespace camera_service;
using namespace testing;

class CoreMock: public core::ICore {
public:
    MOCK_METHOD(Result<void>, initialize, (), (override));
    MOCK_METHOD(Result<void>, shutdown, (), (override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
};

class GrpcIntegrationTests : public Test {
protected:
    void SetUp() override {
        auto core_obj = std::make_unique<CoreMock>();
        core = core_obj.get();

        EXPECT_CALL(*core, initialize())
            .WillOnce(Return(Result<void>::success()));

        request_handler = std::make_shared<api::RequestHandler>(std::move(core_obj));
        grpc_transport = std::make_unique<api::GrpcTransport>(request_handler);

        ASSERT_TRUE(request_handler->start().isSuccess());
        ASSERT_TRUE(grpc_transport->start(server_address).isSuccess());

        // Run the server loop in a separate thread
        server_thread = std::thread([this]() {
            server_result = grpc_transport->runLoop();
        });

        // Give the server a moment to start listening
        std::cout << "Connecting to server at " << server_address << std::endl;
        const auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
        client = std::make_unique<GrpcClient>(channel);
    }

    void TearDown() override {
        EXPECT_CALL(*core, shutdown())
            .WillOnce(Return(Result<void>::success()));

        if (grpc_transport) {
            grpc_transport->stop();
        }

        if (server_thread.joinable()) {
            server_thread.join();
        }
    }

    CoreMock* core {}; // Raw pointer to access the mock
    std::shared_ptr<api::RequestHandler> request_handler;
    std::unique_ptr<api::GrpcTransport> grpc_transport;
    std::string server_address = "0.0.0.0:50051";
    std::unique_ptr<GrpcClient> client;
    std::thread server_thread;
    Result<void> server_result;
};

TEST_F(GrpcIntegrationTests, SetZoomAndGetZoomSuccess) {
    constexpr double test_zoom = 2.5;

    EXPECT_CALL(*core, setZoom(test_zoom))
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*core, getZoom())
        .WillOnce(Return(Result<types::zoom>::success(test_zoom)));

    std::cout << "Test SetZoom " << test_zoom << " getZoom" << std::endl;
    ASSERT_TRUE(client->setZoom(test_zoom).isSuccess());
    auto get_zoom_result = client->getZoom();
    ASSERT_TRUE(get_zoom_result.isSuccess());
    EXPECT_EQ(get_zoom_result.value(), test_zoom);
}

TEST_F(GrpcIntegrationTests, RequestFailOnCoreFail) {
    constexpr double test_zoom = 2.5;

    EXPECT_CALL(*core, setZoom(test_zoom))
        .WillOnce(Return(Result<void>::error("Fail")));

    ASSERT_TRUE(client->setZoom(test_zoom).isError());
}

TEST_F(GrpcIntegrationTests, RequestFailOnTimeout) {
    constexpr double test_zoom = 2.5;

    // Simulate a timeout by not responding
    EXPECT_CALL(*core, setZoom(test_zoom))
        .WillOnce(Invoke([]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return Result<void>::success();
        }));

    auto result = client->setZoom(test_zoom);
    ASSERT_TRUE(result.isError());
    ASSERT_TRUE(result.error().find("Deadline") != std::string::npos);
}