#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "core/CoreFactory.h"

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

class CoreFactoryTests : public Test {
protected:
    static std::unique_ptr<MockCamera> createMockCamera() {
        return std::make_unique<MockCamera>();
    }
};

TEST_F(CoreFactoryTests, CreateCameraCoreSuccess) {
    auto core = core::CoreFactory::createCore("nfov", createMockCamera());
    ASSERT_NE(nullptr, core);
    EXPECT_TRUE(dynamic_cast<core::Core*>(core.get()) != nullptr);
}

TEST_F(CoreFactoryTests, ThrowsOnUnknownType) {
    EXPECT_THROW(
        core::CoreFactory::createCore("unknown", createMockCamera()),
        core::CoreException
    );
}
