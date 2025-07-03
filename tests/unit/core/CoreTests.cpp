#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/Core.h"
#include "data/ICamera.h"
#include "common/types/Result.h"

using namespace camera_service;
using namespace testing;

class MockCamera final : public data::ICamera {
public:
    MOCK_METHOD(Result<void>, connect, (), (override));
    MOCK_METHOD(Result<void>, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
private:
    MOCK_METHOD(bool, isInitialized, (), (const));
};

class CoreTests : public Test {
protected:
    void SetUp() override {
        camera = std::make_unique<MockCamera>();
    }

    std::unique_ptr<MockCamera> camera;
};

TEST_F(CoreTests, CanBeCreated) {
    EXPECT_NO_THROW(core::Core(std::move(camera)));
}

TEST_F(CoreTests, ThrowsOnNullCamera) {
    EXPECT_THROW(core::Core(nullptr), std::invalid_argument);
}

TEST_F(CoreTests, InitializeSuccessWhenDisconnected) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // for shutdown
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));

    core::Core core(std::move(camera));
    const auto result = core.initialize();
    EXPECT_TRUE(result.isSuccess()) << "Failed to initialize: " << result.error();
}

TEST_F(CoreTests, InitializeSuccessWhenAlreadyConnected) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(true))
        .WillOnce(Return(true)); // for shutdown
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));

    core::Core core(std::move(camera));
    const auto result = core.initialize();
    EXPECT_TRUE(result.isSuccess()) << "Failed to initialize: " << result.error();
}

TEST_F(CoreTests, InitializeFailsOnConnectError) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false));
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::error("Failed to connect")));

    core::Core core(std::move(camera));
    const auto result = core.initialize();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Failed to connect");
}

TEST_F(CoreTests, ZoomOperationsSuccess) {
    // Set up all expectations before moving the camera
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // for shutdown
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, setZoom(2.0))
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, getZoom())
        .WillOnce(Return(Result<types::zoom>::success(2.0)));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));


    // Now create the core with the moved camera
    core::Core core(std::move(camera));
    const auto init_result = core.initialize();
    ASSERT_TRUE(init_result.isSuccess()) << "Failed to initialize: " << init_result.error();

    const auto set_result = core.setZoom(2.0);
    EXPECT_TRUE(set_result.isSuccess());

    const auto get_result = core.getZoom();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_EQ(get_result.value(), 2.0);
}

TEST_F(CoreTests, ZoomOperationsFailWhenNotInitialized) {
    core::Core core(std::move(camera));

    const auto set_result = core.setZoom(2.0);
    EXPECT_TRUE(set_result.isError());

    const auto get_result = core.getZoom();
    EXPECT_TRUE(get_result.isError());
}

TEST_F(CoreTests, FocusOperations) {
    // Set up all expectations before moving the camera
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // for shutdown
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, setFocus(1.5))
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, getFocus())
        .WillOnce(Return(Result<types::focus>::success(1.5)));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));

    // Now create the core with the moved camera
    core::Core core(std::move(camera));
    const auto init_result = core.initialize();
    ASSERT_TRUE(init_result.isSuccess());

    const auto set_result = core.setFocus(1.5);
    EXPECT_TRUE(set_result.isSuccess());

    const auto get_result = core.getFocus();
    ASSERT_TRUE(get_result.isSuccess());
    EXPECT_EQ(get_result.value(), 1.5);
}

TEST_F(CoreTests, FocusOperationsFailWhenNotInitialized) {
    core::Core core(std::move(camera));

    const auto set_result = core.setFocus(2.0);
    EXPECT_TRUE(set_result.isError());

    const auto get_result = core.getFocus();
    EXPECT_TRUE(get_result.isError());
}

TEST_F(CoreTests, ShutdownSuccess) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))  // initialize
        .WillOnce(Return(true));  // shutdown
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));

    core::Core core(std::move(camera));
    const auto init_result = core.initialize();
    ASSERT_TRUE(init_result.isSuccess()) << "Failed to initialize: " << init_result.error();

    const auto shutdown_result = core.shutdown();
    EXPECT_TRUE(shutdown_result.isSuccess()) << "Failed to shut down: " << shutdown_result.error();
}

TEST_F(CoreTests, ShutdownWhenNotInitializedSuccess) {
    core::Core core(std::move(camera));
    const auto shutdown_result = core.shutdown();
    EXPECT_TRUE(shutdown_result.isSuccess());
}

TEST_F(CoreTests, ShutdownWhenCameraDisconnectFailsFails) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::error("Failed to disconnect")));

    core::Core core(std::move(camera));
    const auto init_result = core.initialize();
    ASSERT_TRUE(init_result.isSuccess());

    const auto shutdown_result = core.shutdown();
    EXPECT_TRUE(shutdown_result.isError());
    EXPECT_EQ(shutdown_result.error(), "Failed to disconnect");
}