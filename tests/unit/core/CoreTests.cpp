#include "gtest/gtest.h"
#include "gmock/gmock.h"
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

    core::Core core(std::move(camera));
    auto result = core.initialize();
    EXPECT_TRUE(result.isSuccess()) << "Failed to initialize: " << result.error();
}

TEST_F(CoreTests, InitializeSuccessWhenAlreadyConnected) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(true))
        .WillOnce(Return(true)); // for shutdown

    core::Core core(std::move(camera));
    auto result = core.initialize();
    EXPECT_TRUE(result.isSuccess()) << "Failed to initialize: " << result.error();
}

TEST_F(CoreTests, InitializeFailsOnConnectError) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false));
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::error("Failed to connect")));

    core::Core core(std::move(camera));
    auto result = core.initialize();
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), "Failed to connect");
}

TEST_F(CoreTests, ZoomOperations) {
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

    // Now create the core with the moved camera
    core::Core core(std::move(camera));
    auto initResult = core.initialize();
    ASSERT_TRUE(initResult.isSuccess()) << "Failed to initialize: " << initResult.error();

    auto setResult = core.setZoom(2.0);
    EXPECT_TRUE(setResult.isSuccess()) << "Failed to set zoom: " << setResult.error();

    auto getResult = core.getZoom();
    ASSERT_TRUE(getResult.isSuccess()) << "Failed to get zoom: " << getResult.error();
    EXPECT_EQ(getResult.value(), 2.0);
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

    // Now create the core with the moved camera
    core::Core core(std::move(camera));
    auto initResult = core.initialize();
    ASSERT_TRUE(initResult.isSuccess()) << "Failed to initialize: " << initResult.error();

    auto setResult = core.setFocus(1.5);
    EXPECT_TRUE(setResult.isSuccess()) << "Failed to set focus: " << setResult.error();

    auto getResult = core.getFocus();
    ASSERT_TRUE(getResult.isSuccess()) << "Failed to get focus: " << getResult.error();
    EXPECT_EQ(getResult.value(), 1.5);
}

TEST_F(CoreTests, ShutdownSuccess) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))  // for initialize
        .WillOnce(Return(true));  // for shutdown check
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(Result<void>::success()));
    EXPECT_CALL(*camera, disconnect())
        .WillOnce(Return(Result<void>::success()));

    core::Core core(std::move(camera));
    auto initResult = core.initialize();
    ASSERT_TRUE(initResult.isSuccess()) << "Failed to initialize: " << initResult.error();

    auto shutdownResult = core.shutdown();
    EXPECT_TRUE(shutdownResult.isSuccess()) << "Failed to shutdown: " << shutdownResult.error();
}