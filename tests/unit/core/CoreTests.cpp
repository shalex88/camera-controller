#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/Core.h"
#include "data/ICameraHal.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class MockCamera final : public data::ICameraHal {
public:
    MOCK_METHOD(Result<void>, connect, (), (override));
    MOCK_METHOD(Result<void>, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (const, override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (const, override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
    MOCK_METHOD(Result<types::info>, getInfo, (), (const, override));
private:
    MOCK_METHOD(bool, isInitialized, (), (const));
};

class CoreTests : public Test {
protected:
    CoreTests() {
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "Core");
        camera = std::make_unique<MockCamera>();
    }
    std::unique_ptr<MockCamera> camera;
    std::shared_ptr<LayerLogger> logger_impl_;
};

TEST_F(CoreTests, CanBeCreated) {
    EXPECT_NO_THROW(core::Core(std::move(camera), logger_impl_));
}

TEST_F(CoreTests, ThrowsOnNullCamera) {
    EXPECT_THROW(core::Core(nullptr, logger_impl_), std::invalid_argument);
}

TEST_F(CoreTests, InitializeSuccessWhenDisconnected) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // for shutdown
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));

    core::Core core(std::move(camera), logger_impl_);
    const auto result = core.initialize();
    ASSERT_TRUE(result.isSuccess()) << "Failed to initialize: " << result.error();
}

TEST_F(CoreTests, InitializeSuccessWhenAlreadyConnected) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(true))
        .WillOnce(Return(true)); // for shutdown
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));

    core::Core core(std::move(camera), logger_impl_);
    const auto result = core.initialize();
    ASSERT_TRUE(result.isSuccess()) << "Failed to initialize: " << result.error();
}

TEST_F(CoreTests, InitializeFailsOnConnectError) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false));
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::error("Failed to connect")));

    core::Core core(std::move(camera), logger_impl_);
    const auto result = core.initialize();
    ASSERT_TRUE(result.isError());
    EXPECT_THAT(result.error(), ::testing::HasSubstr("Failed to connect"));
}

TEST_F(CoreTests, ZoomOperationsSuccess) {
    // Set up all expectations before moving the camera
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // for shutdown
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, setZoom(2))
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, getZoom())
        .WillOnce(Return(Result<types::zoom>::success(2u)));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));


    // Now create the core with the moved camera
    core::Core core(std::move(camera), logger_impl_);
    const auto init_result = core.initialize();
    ASSERT_TRUE(init_result.isSuccess()) << "Failed to initialize: " << init_result.error();

    const auto set_result = core.setZoom(2);
    ASSERT_TRUE(set_result.isSuccess());

    const auto get_result = core.getZoom();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_EQ(get_result.value(), 2);
}

TEST_F(CoreTests, ZoomOperationsFailWhenNotInitialized) {
    core::Core core(std::move(camera), logger_impl_);

    const auto set_result = core.setZoom(2);
    ASSERT_TRUE(set_result.isError());

    const auto get_result = core.getZoom();
    ASSERT_TRUE(get_result.isError());
}

TEST_F(CoreTests, FocusOperations) {
    // Set up all expectations before moving the camera
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // for shutdown
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, setFocus(1))
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, getFocus())
        .WillOnce(Return(Result<types::focus>::success(1u)));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));

    // Now create the core with the moved camera
    core::Core core(std::move(camera), logger_impl_);
    const auto init_result = core.initialize();
    ASSERT_TRUE(init_result.isSuccess());

    const auto set_result = core.setFocus(1);
    ASSERT_TRUE(set_result.isSuccess());

    const auto get_result = core.getFocus();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_EQ(get_result.value(), 1);
}

TEST_F(CoreTests, FocusOperationsFailWhenNotInitialized) {
    core::Core core(std::move(camera), logger_impl_);

    const auto set_result = core.setFocus(2);
    ASSERT_TRUE(set_result.isError());

    const auto get_result = core.getFocus();
    ASSERT_TRUE(get_result.isError());
}

TEST_F(CoreTests, ShutdownSuccess) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))  // initialize
        .WillOnce(Return(true));  // shutdown
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));

    core::Core core(std::move(camera), logger_impl_);
    const auto init_result = core.initialize();
    ASSERT_TRUE(init_result.isSuccess()) << "Failed to initialize: " << init_result.error();

    const auto shutdown_result = core.shutdown();
    ASSERT_TRUE(shutdown_result.isSuccess()) << "Failed to shut down: " << shutdown_result.error();
}

TEST_F(CoreTests, ShutdownWhenNotInitializedSuccess) {
    core::Core core(std::move(camera), logger_impl_);
    const auto shutdown_result = core.shutdown();
    ASSERT_TRUE(shutdown_result.isSuccess());
}

TEST_F(CoreTests, ShutdownWhenCameraDisconnectFailsFails) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::error("Failed to disconnect")));

    core::Core core(std::move(camera), logger_impl_);
    const auto init_result = core.initialize();
    ASSERT_TRUE(init_result.isSuccess());

    const auto shutdown_result = core.shutdown();
    ASSERT_TRUE(shutdown_result.isError());
    EXPECT_EQ(shutdown_result.error(), "Failed to disconnect");
}