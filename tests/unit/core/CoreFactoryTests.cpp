#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "core/CoreFactory.h"

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
        std::invalid_argument
    );
}

TEST_F(CoreFactoryTests, ThrowsOnNullCamera) {
    EXPECT_THROW(
        core::CoreFactory::createCore("nfov", nullptr),
        std::invalid_argument
    );
}
