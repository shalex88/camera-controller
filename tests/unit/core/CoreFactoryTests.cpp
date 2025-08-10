#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "core/CoreFactory.h"

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
    MOCK_METHOD(Result<void>, setZoom, (types::zoom), (override));
    MOCK_METHOD(Result<types::zoom>, getZoom, (), (const, override));
    MOCK_METHOD(Result<void>, setFocus, (types::focus), (override));
    MOCK_METHOD(Result<types::focus>, getFocus, (), (const, override));
};

class CoreFactoryTests : public Test {
protected:
    CoreFactoryTests() {
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "API");
        core_ = std::make_unique<MockCamera>();
    }
    std::shared_ptr<LayerLogger> logger_impl_;
    std::unique_ptr<MockCamera> core_;
};

TEST_F(CoreFactoryTests, CreateCameraCoreSuccess) {
    const auto core = core::CoreFactory::createCore("nfov", std::move(core_), logger_impl_);
    ASSERT_NE(nullptr, core);
    ASSERT_TRUE(dynamic_cast<core::Core*>(core.get()) != nullptr);
}

TEST_F(CoreFactoryTests, ThrowsOnUnknownType) {
    EXPECT_THROW(
        core::CoreFactory::createCore("unknown", std::move(core_), logger_impl_),
        std::invalid_argument
    );
}

TEST_F(CoreFactoryTests, ThrowsOnNullCamera) {
    EXPECT_THROW(
        core::CoreFactory::createCore("nfov", nullptr, logger_impl_),
        std::invalid_argument
    );
}
