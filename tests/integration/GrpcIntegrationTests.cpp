#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include <chrono>
#include <memory>
#include <thread>

#include "api/GrpcTransport.h"
#include "api/RequestHandler.h"
#include "../../utils/GrpcClient.h"
#include "../Mocks.h"

class GrpcIntegrationTests : public Test {
protected:
    void SetUp() override {
        auto core_obj = std::make_unique<CoreMock>();
        core = core_obj.get();

        EXPECT_CALL(*core, initialize())
            .WillOnce(Return(Result<void>::success()));

        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "");
        request_handler = std::make_shared<api::RequestHandler>(std::move(core_obj), logger_impl_);
        grpc_transport = std::make_unique<api::GrpcTransport>(request_handler, logger_impl_);

        ASSERT_TRUE(request_handler->start().isSuccess());
        ASSERT_TRUE(grpc_transport->start(server_address).isSuccess());

        // Run the server loop in a separate thread
        server_thread = std::thread([this] {
            server_result = grpc_transport->runLoop();
        });

        // Give the server a moment to start listening
        std::cout << "Connecting to server at " << server_address << std::endl;
        const auto channel = CreateChannel(server_address, grpc::InsecureChannelCredentials());
        client = std::make_unique<GrpcClient>(channel);
    }

    void TearDown() override {
        EXPECT_CALL(*core, shutdown())
            .WillOnce(Return(Result<void>::success()));

        if (grpc_transport) {
            ASSERT_TRUE(grpc_transport->stop().isSuccess());
        }

        if (server_thread.joinable()) {
            server_thread.join();
        }
    }

    CoreMock* core {}; // Raw pointer to access the mock
    std::shared_ptr<api::IRequestHandler> request_handler;
    std::unique_ptr<api::GrpcTransport> grpc_transport;
    std::string server_address = "0.0.0.0:50051";
    std::unique_ptr<GrpcClient> client;
    std::thread server_thread;
    Result<void> server_result;
    std::shared_ptr<LayerLogger> logger_impl_;
};

TEST_F(GrpcIntegrationTests, SetZoomAndGetZoomSuccess) {
    constexpr uint32_t test_zoom = 3u;  // Use uint32_t instead of double

    EXPECT_CALL(*core, setZoom(test_zoom))
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*core, getZoom())
        .WillOnce(Return(Result<types::zoom>::success(test_zoom)));

    std::cout << "Test SetZoom " << test_zoom << " getZoom" << std::endl;
    ASSERT_TRUE(client->setZoom(test_zoom).isSuccess());
    const auto get_zoom_result = client->getZoom();
    ASSERT_TRUE(get_zoom_result.isSuccess());
    EXPECT_EQ(get_zoom_result.value(), test_zoom);
}

TEST_F(GrpcIntegrationTests, RequestFailOnCoreFail) {
    constexpr uint32_t test_zoom = 3u;  // Use uint32_t instead of double

    EXPECT_CALL(*core, setZoom(test_zoom))
        .WillOnce(Return(Result<void>::error("Fail")));

    ASSERT_TRUE(client->setZoom(test_zoom).isError());
}

TEST_F(GrpcIntegrationTests, RequestFailOnTimeout) {
    constexpr uint32_t test_zoom = 3u;  // Use uint32_t instead of double

    // Simulate a timeout by not responding
    EXPECT_CALL(*core, setZoom(test_zoom))
        .WillOnce(Invoke([] {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return Result<void>::success();
        }));

    const auto result = client->setZoom(test_zoom);
    ASSERT_TRUE(result.isError());
    ASSERT_TRUE(result.error().find("Deadline") != std::string::npos);
}