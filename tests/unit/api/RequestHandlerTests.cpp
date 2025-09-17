#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "api/RequestHandler.h"
#include "core/ICore.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class CoreMock : public core::ICore {
public:
    MOCK_METHOD(Result<void>, initialize, (), (override));
    MOCK_METHOD(Result<void>, shutdown, (), (override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
};

class RequestHandlerTests : public Test {
protected:
    RequestHandlerTests() {
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "API");
        core = new NiceMock<CoreMock>();
        auto core_obj = std::unique_ptr<core::ICore>(core);
        request_handler = std::make_unique<api::RequestHandler>(std::move(core_obj), logger_impl_);
    }
    std::unique_ptr<api::RequestHandler> request_handler;
    NiceMock<CoreMock>* core {};
    std::shared_ptr<LayerLogger> logger_impl_;
};

TEST_F(RequestHandlerTests, CreationSuccess) {
    ASSERT_NE(nullptr, request_handler);
}

TEST_F(RequestHandlerTests, CreationFailNoCore) {
    EXPECT_THROW(api::RequestHandler request_handler(nullptr, logger_impl_), std::invalid_argument);
}

TEST_F(RequestHandlerTests, StartSuccess) {
    const auto result = request_handler->start();
    ASSERT_TRUE(result.isSuccess());
}

TEST_F(RequestHandlerTests, StartFailOnInitialize) {
    EXPECT_CALL(*core, initialize())
        .WillOnce(Return(Result<void>::error("Initialize failed")));

    const auto result = request_handler->start();
    ASSERT_TRUE(result.isError());
}

TEST_F(RequestHandlerTests, StopSuccessIfRunning) {
    EXPECT_CALL(*core, initialize())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*core, shutdown())
        .WillOnce(Return(Result<void>::success()));

    const auto start_result = request_handler->start();
    ASSERT_TRUE(start_result.isSuccess());

    const auto stop_result = request_handler->stop();
    ASSERT_TRUE(stop_result.isSuccess());
}

TEST_F(RequestHandlerTests, StopSuccessIfNotRunning) {
    const auto result = request_handler->stop();
    ASSERT_TRUE(result.isSuccess());
}

TEST_F(RequestHandlerTests, StopFailsIfCoreShutdownFails) {
    EXPECT_CALL(*core, initialize())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*core, shutdown())
        .WillOnce(Return(Result<void>::error("Shutdown failed")));

    const auto start_result = request_handler->start();
    ASSERT_TRUE(start_result.isSuccess());

    const auto stop_result = request_handler->stop();
    ASSERT_TRUE(stop_result.isError());
}

TEST_F(RequestHandlerTests, ZoomOperations) {
    Sequence s;
    EXPECT_CALL(*core, initialize())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*core, setZoom(2))
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*core, getZoom())
        .InSequence(s)
        .WillOnce(Return(Result<types::zoom>::success(2u)));
    EXPECT_CALL(*core, shutdown())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));

    const auto start_result = request_handler->start();
    ASSERT_TRUE(start_result.isSuccess()) << "Failed to start: " << start_result.error();

    const auto set_result = request_handler->setZoom(2);
    ASSERT_TRUE(set_result.isSuccess()) << "Failed to set zoom: " << set_result.error();

    const auto get_result = request_handler->getZoom();
    ASSERT_TRUE(get_result.isSuccess()) << "Failed to get zoom: " << get_result.error();
    EXPECT_EQ(2, get_result.value());

    const auto stop_result = request_handler->stop();
    ASSERT_TRUE(stop_result.isSuccess()) << "Failed to stop: " << stop_result.error();
}

TEST_F(RequestHandlerTests, ZoomOperationsFailIfNotRunning) {
    const auto set_result = request_handler->setZoom(2);
    ASSERT_TRUE(set_result.isError());

    const auto get_result = request_handler->getZoom();
    ASSERT_TRUE(get_result.isError());
}

TEST_F(RequestHandlerTests, FocusOperations) {
    Sequence s;
    EXPECT_CALL(*core, initialize())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*core, setFocus(1))
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*core, getFocus())
        .InSequence(s)
        .WillOnce(Return(Result<types::focus>::success(1u)));
    EXPECT_CALL(*core, shutdown())
        .InSequence(s)
        .WillOnce(Return(Result<void>::success()));

    const auto start_result = request_handler->start();
    ASSERT_TRUE(start_result.isSuccess()) << "Failed to start: " << start_result.error();

    const auto set_result = request_handler->setFocus(1);
    ASSERT_TRUE(set_result.isSuccess()) << "Failed to set focus: " << set_result.error();

    const auto get_result = request_handler->getFocus();
    ASSERT_TRUE(get_result.isSuccess()) << "Failed to get focus: " << get_result.error();
    EXPECT_EQ(1, get_result.value());

    const auto stop_result = request_handler->stop();
    ASSERT_TRUE(stop_result.isSuccess()) << "Failed to stop: " << stop_result.error();
}

TEST_F(RequestHandlerTests, FocusOperationsFailIfNotRunning) {
    const auto set_result = request_handler->setFocus(2);
    ASSERT_TRUE(set_result.isError());

    const auto get_result = request_handler->getFocus();
    ASSERT_TRUE(get_result.isError());
}
