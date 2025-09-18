#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "core/CoreFactory.h"
#include "common/Config/ConfigManager.h"

#include "core/Core.h"
#include "../../Mocks.h"
#include "common/types/Result.h"

class CoreFactoryTests : public Test {
protected:
    CoreFactoryTests() {
        logger_impl_ = std::make_shared<LayerLogger>(std::make_shared<SpdLogAdapter>(), "API");
        core_ = std::make_unique<MockCameraHal>();
    }
    std::shared_ptr<LayerLogger> logger_impl_;
    std::unique_ptr<MockCameraHal> core_;
};

TEST_F(CoreFactoryTests, CreateCameraCoreSuccess) {
    CoreConfig config;
    config.camera = "nfov";  // Valid camera type

    const auto core = core::CoreFactory::createCore(std::move(core_), logger_impl_, config);
    ASSERT_NE(nullptr, core);
    ASSERT_TRUE(dynamic_cast<core::Core*>(core.get()) != nullptr);
}

TEST_F(CoreFactoryTests, ThrowsOnUnknownType) {
    CoreConfig config;
    config.camera = "invalid_camera";  // Invalid camera type to trigger exception

    EXPECT_THROW(
        core::CoreFactory::createCore(std::move(core_), logger_impl_, config),
        std::invalid_argument
    );
}

TEST_F(CoreFactoryTests, ThrowsOnNullCamera) {
    CoreConfig config;
    config.camera = "nfov";  // Valid camera type

    EXPECT_THROW(
        core::CoreFactory::createCore(nullptr, logger_impl_, config),
        std::invalid_argument
    );
}
