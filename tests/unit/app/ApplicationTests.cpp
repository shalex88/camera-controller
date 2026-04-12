#include <csignal>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "app/Application.h"
#include "common/logger/Logger.h"
#include "common/logger/LoggerInterface.h"
#include "common/logger/SpdLogAdapter.h"

using namespace testing;

namespace {
    class MockLoggerAdapter : public LoggerInterface {
    public:
        MOCK_METHOD(void, setLogLevel, (LogLevel), (override));
        MOCK_METHOD(void, setLogLevel, (const std::string&), (override));
        MOCK_METHOD(void, logImpl, (LogLevel, const std::string&), (override));
    };

    class ApplicationTests : public Test {
    protected:
        void SetUp() override {
            auto mock = std::make_shared<NiceMock<MockLoggerAdapter>>();
            mock_logger_ = mock.get();
            service::common::LoggerRegistry::instance().setLoggerAdapter(std::move(mock), "info");
        }

        void TearDown() override {
            std::signal(SIGINT, SIG_DFL);
            std::signal(SIGTERM, SIG_DFL);
            service::common::LoggerRegistry::instance().setLoggerAdapter(
                std::make_shared<SpdLogAdapter>(), "info");
        }

        NiceMock<MockLoggerAdapter>* mock_logger_ {};
    };
} // namespace

TEST_F(ApplicationTests, RunRequestsShutdownWhenSigIntWasReceived) {
    char app_name[] = "camera-controller";
    char* argv[] = {app_name};
    service::app::Application app(1, argv);

    EXPECT_EQ(std::raise(SIGINT), 0);
    EXPECT_CALL(*mock_logger_, logImpl(LoggerInterface::LogLevel::Info, HasSubstr("Stopping...")));

    app.run();
}
