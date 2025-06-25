#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "api/Controller.h"
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

class GrpcControllerTests : public Test {
protected:
    void SetUp() override {
        mock_core = std::make_unique<MockCore>();
    }

    std::unique_ptr<MockCore> mock_core;
    std::string port = "50051";
};

TEST_F(GrpcControllerTests, CreationSuccess) {
    // Just verify we can create it - no need for shutdown expectation
    // since we're not starting the controller or moving the mock
    ASSERT_NE(nullptr, std::make_unique<api::Controller>(
        std::make_unique<MockCore>(),  // Create a new mock instead of moving the test's mock
        std::make_unique<api::GrpcTransport>(),
        port));
}

TEST_F(GrpcControllerTests, CreationFailNoCore) {
    // Using nullptr, so no need to move mock_core
    EXPECT_THROW(api::Controller controller(nullptr, std::make_unique<api::GrpcTransport>(), port), std::invalid_argument);
}

TEST_F(GrpcControllerTests, CreationFailNoControllerImpl) {
    // Create a new mock since this is a failure test
    EXPECT_THROW(api::Controller controller(std::make_unique<MockCore>(), nullptr, port), std::invalid_argument);
}

TEST_F(GrpcControllerTests, CreationFailNoPort) {
    // Create a new mock since this is a failure test
    EXPECT_THROW(api::Controller controller(std::make_unique<MockCore>(), std::make_unique<api::GrpcTransport>(), ""), std::invalid_argument);
}

TEST_F(GrpcControllerTests, StartSuccess) {
    EXPECT_CALL(*mock_core, initialize())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*mock_core, shutdown())
        .WillOnce(Return(Result<void>::success()));

    const auto controller = std::make_unique<api::Controller>(std::move(mock_core), std::make_unique<api::GrpcTransport>(), port);
    const auto result = controller->startAsync();
    EXPECT_TRUE(result.isSuccess()) << "Failed to start: " << result.error();
}

TEST_F(GrpcControllerTests, StartFailOnInitialize) {
    EXPECT_CALL(*mock_core, initialize())
        .WillOnce(Return(Result<void>::error("Initialize failed")));
    // No shutdown expectation needed here since initialize fails

    const auto controller = std::make_unique<api::Controller>(std::move(mock_core), std::make_unique<api::GrpcTransport>(), port);
    const auto result = controller->startAsync();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Core initialization failed: Initialize failed");
}

TEST_F(GrpcControllerTests, StopSuccess) {
    // Add explicit order to ensure shutdown happens after start
    const Sequence s;
    EXPECT_CALL(*mock_core, initialize())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*mock_core, shutdown())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));

    const auto controller = std::make_unique<api::Controller>(std::move(mock_core), std::make_unique<api::GrpcTransport>(), port);
    const auto start_result = controller->startAsync();
    ASSERT_TRUE(start_result.isSuccess()) << "Failed to start: " << start_result.error();

    // Give the transport time to start before stopping
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto stop_result = controller->stop();
    EXPECT_TRUE(stop_result.isSuccess()) << "Failed to stop: " << stop_result.error();

    // Give the transport time to fully clean up before destroying
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(GrpcControllerTests, ZoomOperations) {
    // Add explicit order to ensure operations happen in sequence
    Sequence s;
    EXPECT_CALL(*mock_core, initialize())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*mock_core, setZoom(2.0))
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*mock_core, getZoom())
        .InSequence(s)
        .WillOnce(Return(Result<types::zoom>::success(2.0)));
    EXPECT_CALL(*mock_core, shutdown())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));

    const auto controller = std::make_unique<api::Controller>(std::move(mock_core), std::make_unique<api::GrpcTransport>(), port);
    const auto start_result = controller->startAsync();
    ASSERT_TRUE(start_result.isSuccess()) << "Failed to start: " << start_result.error();

    // Give the transport time to start before operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto set_result = controller->setZoom(2.0);
    EXPECT_TRUE(set_result.isSuccess()) << "Failed to set zoom: " << set_result.error();

    const auto get_result = controller->getZoom();
    ASSERT_TRUE(get_result.isSuccess()) << "Failed to get zoom: " << get_result.error();
    EXPECT_DOUBLE_EQ(2.0, get_result.value());

    const auto stop_result = controller->stop();
    EXPECT_TRUE(stop_result.isSuccess()) << "Failed to stop: " << stop_result.error();

    // Give the transport time to fully clean up before destroying
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(GrpcControllerTests, FocusOperations) {
    // Add explicit order to ensure operations happen in sequence
    Sequence s;
    EXPECT_CALL(*mock_core, initialize())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*mock_core, setFocus(1.5))
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*mock_core, getFocus())
        .InSequence(s)
        .WillOnce(Return(Result<types::focus>::success(1.5)));
    EXPECT_CALL(*mock_core, shutdown())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));

    const auto controller = std::make_unique<api::Controller>(std::move(mock_core), std::make_unique<api::GrpcTransport>(), port);
    const auto start_result = controller->startAsync();
    ASSERT_TRUE(start_result.isSuccess()) << "Failed to start: " << start_result.error();

    // Give the transport time to start before operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto set_result = controller->setFocus(1.5);
    EXPECT_TRUE(set_result.isSuccess()) << "Failed to set focus: " << set_result.error();

    const auto get_result = controller->getFocus();
    ASSERT_TRUE(get_result.isSuccess()) << "Failed to get focus: " << get_result.error();
    EXPECT_DOUBLE_EQ(1.5, get_result.value());

    const auto stop_result = controller->stop();
    EXPECT_TRUE(stop_result.isSuccess()) << "Failed to stop: " << stop_result.error();

    // Give the transport time to fully clean up before destroying
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}