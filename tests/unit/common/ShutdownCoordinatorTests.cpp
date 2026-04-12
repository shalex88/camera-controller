#include <gtest/gtest.h>

#include "common/runtime/ShutdownCoordinator.h"

namespace service::common::runtime {
    class ShutdownCoordinatorTests : public ::testing::Test {
    protected:
        void SetUp() override {
            clearShutdownHandler();
        }

        void TearDown() override {
            clearShutdownHandler();
        }
    };

    TEST_F(ShutdownCoordinatorTests, RequestShutdownInvokesRegisteredHandler) {
        bool shutdown_requested = false;

        registerShutdownHandler([&shutdown_requested] {
            shutdown_requested = true;
        });

        requestShutdown();

        EXPECT_TRUE(shutdown_requested);
    }

    TEST_F(ShutdownCoordinatorTests, ClearShutdownHandlerPreventsInvocation) {
        bool shutdown_requested = false;

        registerShutdownHandler([&shutdown_requested] {
            shutdown_requested = true;
        });
        clearShutdownHandler();

        requestShutdown();

        EXPECT_FALSE(shutdown_requested);
    }

    TEST_F(ShutdownCoordinatorTests, RequestShutdownWithoutRegisteredHandlerDoesNotCrash) {
        EXPECT_NO_THROW(requestShutdown());
    }
} // namespace service::common::runtime
