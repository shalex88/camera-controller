#include "gtest/gtest.h"
#include "gmock/gmock.h"
/* Add your project include files here */
#include "common/Logger/Logger.h"

#include "common/Logger/LoggerInterface.h"

using namespace testing;

class MockLoggerAdapter : public LoggerInterface {
public:
    MOCK_METHOD(void, setLogLevel, (LogLevel), (override));
    MOCK_METHOD(void, logImpl, (LogLevel, const std::string&), (override));
};

class LoggerTest: public Test {
protected:
    void SetUp() override {
        auto mock = std::make_unique<NiceMock<MockLoggerAdapter>>(); // Prevent side effects caused by the singleton
        mock_logger = mock.get(); // Keep a raw pointer for expectations
        Logger::getInstance().setLoggerAdapter(std::move(mock));
    }

    void TearDown() override {
        Logger::getInstance().setLoggerAdapter(std::make_unique<SpdLogAdapter>()); // Reset the logger adapter to avoid side effects
    }

    NiceMock<MockLoggerAdapter>* mock_logger{};
};

TEST_F(LoggerTest, SetLogLevel) {
    EXPECT_CALL(*mock_logger, setLogLevel(LoggerInterface::LogLevel::Debug));
    SET_LOG_LEVEL(LoggerInterface::LogLevel::Debug);
}

TEST_F(LoggerTest, LogTrace) {
    EXPECT_CALL(*mock_logger, logImpl(LoggerInterface::LogLevel::Trace, "Test message"));
    LOG_TRACE("Test message");
}

TEST_F(LoggerTest, LogDebug) {
    EXPECT_CALL(*mock_logger, logImpl(LoggerInterface::LogLevel::Debug, "Test message"));
    LOG_DEBUG("Test message");
}

TEST_F(LoggerTest, LogInfo) {
    EXPECT_CALL(*mock_logger, logImpl(LoggerInterface::LogLevel::Info, "Test message"));
    LOG_INFO("Test message");
}

TEST_F(LoggerTest, LogWarn) {
    EXPECT_CALL(*mock_logger, logImpl(LoggerInterface::LogLevel::Warn, "Test message"));
    LOG_WARN("Test message");
}

TEST_F(LoggerTest, LogError) {
    EXPECT_CALL(*mock_logger, logImpl(LoggerInterface::LogLevel::Error, "Test message"));
    LOG_ERROR("Test message");
}

TEST_F(LoggerTest, LogCritical) {
    EXPECT_CALL(*mock_logger, logImpl(LoggerInterface::LogLevel::Critical, "Test message"));
    LOG_CRITICAL("Test message");
}

TEST_F(LoggerTest, LogWithFormatting) {
    EXPECT_CALL(*mock_logger, logImpl(LoggerInterface::LogLevel::Info, "Value: 42"));
    LOG_INFO("Value: {}", 42);
}

TEST_F(LoggerTest, LogWithMultipleArgs) {
    EXPECT_CALL(*mock_logger, logImpl(LoggerInterface::LogLevel::Info, "Hello World 42"));
    LOG_INFO("{} {} {}", "Hello", "World", 42);
}
