#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/RequestHandler.h"
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

class RequestHandlerTests : public Test {
protected:
    void SetUp() override {
        mock_core = std::make_unique<MockCore>();
    }

    std::unique_ptr<MockCore> mock_core;
};

TEST_F(RequestHandlerTests, CreationSuccess) {
    ASSERT_NE(nullptr, std::make_unique<api::RequestHandler>(std::make_unique<MockCore>()));
}

TEST_F(RequestHandlerTests, CreationFailNoCore) {
    EXPECT_THROW(api::RequestHandler request_handler(nullptr), std::invalid_argument);
}

TEST_F(RequestHandlerTests, StartSuccess) {
    EXPECT_CALL(*mock_core, initialize())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*mock_core, shutdown())
        .WillOnce(Return(Result<void>::success()));

    const auto request_handler = std::make_unique<api::RequestHandler>(std::move(mock_core));
    const auto result = request_handler->startAsync();
    EXPECT_TRUE(result.isSuccess()) << "Failed to start: " << result.error();
}

TEST_F(RequestHandlerTests, StartFailOnInitialize) {
    EXPECT_CALL(*mock_core, initialize())
        .WillOnce(Return(Result<void>::error("Initialize failed")));

    const auto request_handler = std::make_unique<api::RequestHandler>(std::move(mock_core));
    const auto result = request_handler->startAsync();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Core initialization failed: Initialize failed");
}

TEST_F(RequestHandlerTests, StopSuccess) {
    const Sequence s;
    EXPECT_CALL(*mock_core, initialize())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*mock_core, shutdown())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));

    const auto request_handler = std::make_unique<api::RequestHandler>(std::move(mock_core));
    const auto start_result = request_handler->startAsync();
    ASSERT_TRUE(start_result.isSuccess()) << "Failed to start: " << start_result.error();

    const auto stop_result = request_handler->stop();
    EXPECT_TRUE(stop_result.isSuccess()) << "Failed to stop: " << stop_result.error();
}

TEST_F(RequestHandlerTests, ZoomOperations) {
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

    const auto request_handler = std::make_unique<api::RequestHandler>(std::move(mock_core));
    const auto start_result = request_handler->startAsync();
    ASSERT_TRUE(start_result.isSuccess()) << "Failed to start: " << start_result.error();

    const auto set_result = request_handler->setZoom(2.0);
    EXPECT_TRUE(set_result.isSuccess()) << "Failed to set zoom: " << set_result.error();

    const auto get_result = request_handler->getZoom();
    ASSERT_TRUE(get_result.isSuccess()) << "Failed to get zoom: " << get_result.error();
    EXPECT_DOUBLE_EQ(2.0, get_result.value());

    const auto stop_result = request_handler->stop();
    EXPECT_TRUE(stop_result.isSuccess()) << "Failed to stop: " << stop_result.error();
}

TEST_F(RequestHandlerTests, FocusOperations) {
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

    const auto request_handler = std::make_unique<api::RequestHandler>(std::move(mock_core));
    const auto start_result = request_handler->startAsync();
    ASSERT_TRUE(start_result.isSuccess()) << "Failed to start: " << start_result.error();

    const auto set_result = request_handler->setFocus(1.5);
    EXPECT_TRUE(set_result.isSuccess()) << "Failed to set focus: " << set_result.error();

    const auto get_result = request_handler->getFocus();
    ASSERT_TRUE(get_result.isSuccess()) << "Failed to get focus: " << get_result.error();
    EXPECT_DOUBLE_EQ(1.5, get_result.value());

    const auto stop_result = request_handler->stop();
    EXPECT_TRUE(stop_result.isSuccess()) << "Failed to stop: " << stop_result.error();
}
