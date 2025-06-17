#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "core/Core.h"

#include "data/ICamera.h"

using namespace camera_service;
using namespace testing;

class MockCamera final : public data::ICamera {
public:
    MOCK_METHOD(bool, connect, (), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(void, setZoom, (double), (override));
    MOCK_METHOD(double, getZoom, (), (const, override));
    MOCK_METHOD(void, setFocus, (double), (override));
    MOCK_METHOD(double, getFocus, (), (const, override));
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
    EXPECT_THROW(core::Core(nullptr), core::CoreException);
}

TEST_F(CoreTests, InitializeSuccessWhenDisconnected) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));

    core::Core core(std::move(camera));
    EXPECT_TRUE(core.initialize());
}

TEST_F(CoreTests, InitializeSuccessWhenAlreadyConnected) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(true))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .Times(0);

    core::Core core(std::move(camera));
    EXPECT_TRUE(core.initialize());
}

TEST_F(CoreTests, InitializeThrowsOnConnectionFailure) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false));
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(false));

    core::Core core(std::move(camera));
    EXPECT_THROW(core.initialize(), core::CoreException);
}

TEST_F(CoreTests, ShutdownWhenNotInitialized) {
    EXPECT_CALL(*camera, isConnected())
        .Times(0);
    EXPECT_CALL(*camera, disconnect())
        .Times(0);

    core::Core core(std::move(camera));
    EXPECT_NO_THROW(core.shutdown());
}

TEST_F(CoreTests, ShutdownDisconnectsWhenConnected) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, disconnect())
        .Times(1);

    core::Core core(std::move(camera));
    core.initialize();
    core.shutdown();
}

TEST_F(CoreTests, SetZoomSuccess) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, setZoom(2.0))
        .Times(1);

    core::Core core(std::move(camera));
    core.initialize();
    core.setZoom(2.0);
}

TEST_F(CoreTests, ThrowOnSetZoomFailure) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, setZoom(_))
        .WillOnce(Throw(data::CameraException("Zoom error")));

    core::Core core(std::move(camera));
    core.initialize();
    EXPECT_THROW(core.setZoom(2.0), core::CoreException);
}

TEST_F(CoreTests, GetZoomSuccess) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, getZoom())
        .WillOnce(Return(2.0));

    core::Core core(std::move(camera));
    core.initialize();
    EXPECT_DOUBLE_EQ(2.0, core.getZoom());
}

TEST_F(CoreTests, TrowOnGetZoomFailure) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, getZoom())
        .WillOnce(Throw(data::CameraException("Zoom error")));

    core::Core core(std::move(camera));
    core.initialize();
    EXPECT_THROW(core.getZoom(), core::CoreException);
}

TEST_F(CoreTests, SetFocusSuccess) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, setFocus(1.5))
        .Times(1);

    core::Core core(std::move(camera));
    core.initialize();
    core.setFocus(1.5);
}

TEST_F(CoreTests, TrowOnCameraSetFocusFailure) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, setFocus(_))
        .WillOnce(Throw(data::CameraException("Focus error")));

    core::Core core(std::move(camera));
    core.initialize();
    EXPECT_THROW(core.setFocus(1.5), core::CoreException);
}

TEST_F(CoreTests, GetFocusSuccess) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, getFocus())
        .WillOnce(Return(1.5));

    core::Core core(std::move(camera));
    core.initialize();
    EXPECT_DOUBLE_EQ(1.5, core.getFocus());
}

TEST_F(CoreTests, TrowOnCameraGetFocusFailure) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, getFocus())
        .WillOnce(Throw(data::CameraException("Focus error")));

    core::Core core(std::move(camera));
    core.initialize();
    EXPECT_THROW(core.getFocus(), core::CoreException);
}

TEST_F(CoreTests, ThrowOnCameraSetFocusFailure) {
    EXPECT_CALL(*camera, isConnected())
        .WillOnce(Return(false))
        .WillOnce(Return(true)); // shutdown()
    EXPECT_CALL(*camera, connect())
        .WillOnce(Return(true));
    EXPECT_CALL(*camera, setFocus(_))
        .WillOnce(Throw(data::CameraException("Focus error")));

    core::Core core(std::move(camera));
    core.initialize();
    EXPECT_THROW(core.setFocus(2.0), core::CoreException);
}

TEST_F(CoreTests, OperationsThrowWhenNotInitialized) {
    core::Core core(std::move(camera));

    EXPECT_THROW(core.setZoom(1.0), core::CoreException);
    EXPECT_THROW(core.getZoom(), core::CoreException);
    EXPECT_THROW(core.setFocus(1.0), core::CoreException);
    EXPECT_THROW(core.getFocus(), core::CoreException);
}