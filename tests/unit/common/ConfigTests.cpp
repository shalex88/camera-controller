#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "common/config/ConfigManager.h"

#include <filesystem>
#include <fstream>

using namespace testing;

class ConfigManagerTests : public Test {
protected:
    const std::string test_config_path_ = "test_config.yaml";
    const std::string invalid_config_path_ = "invalid_config.yaml";

    void SetUp() override {
        // Create a valid test config file with nested structure
        std::ofstream config_file(test_config_path_);
        config_file << "app:\n";
        config_file << "  name: test\n";
        config_file << "  log_level: info\n";
        config_file << "  api:\n";
        config_file << "    api_type: grpc\n";
        config_file << "    server_address: localhost:50051\n";
        config_file << "  core:\n";
        config_file << "    camera: nfov\n";
        config_file << "  data:\n";
        config_file << "    camera: sony\n";
        config_file << "    device: fake\n";
        config_file.close();
    }

    void TearDown() override {
        if (std::filesystem::exists(test_config_path_)) {
            std::filesystem::remove(test_config_path_);
        }
        if (std::filesystem::exists(invalid_config_path_)) {
            std::filesystem::remove(invalid_config_path_);
        }
    }

    void createInvalidConfig(const std::string& content) const {
        std::ofstream config_file(invalid_config_path_);
        config_file << content;
        config_file.close();
    }
};

TEST_F(ConfigManagerTests, LoadValidConfig) {
    const camera_service::common::ConfigManager config(test_config_path_);

    const auto& api_config = config.getApiConfig();
    EXPECT_EQ(api_config.api, "grpc");
    EXPECT_EQ(api_config.server_address, "localhost:50051");

    const auto& core_config = config.getCoreConfig();
    EXPECT_EQ(core_config.camera, "nfov");

    const auto& data_config = config.getDataConfig();
    EXPECT_EQ(data_config.camera, "sony");
    EXPECT_EQ(data_config.device, "fake");

    const auto& log_level = config.getLogLevel();
    const auto& app_name = config.getAppName();
    EXPECT_EQ(app_name, "test");
    EXPECT_EQ(log_level, "info");
}

TEST_F(ConfigManagerTests, ThrowsOnNonexistentFile) {
    EXPECT_THROW({ camera_service::common::ConfigManager config("nonexistent.yaml");
                 }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnInvalidYaml) {
    createInvalidConfig("invalid: : yaml : content");
    EXPECT_THROW({ camera_service::common::ConfigManager config(invalid_config_path_);
                 }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnInvalidApiType) {
    createInvalidConfig("app:\n  name: test\n  log_level: info\n  api:\n    api_type: invalid_api\n    server_address: localhost:50051\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: fake");
    EXPECT_THROW({ camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnMissingServerAddress) {
    createInvalidConfig("app:\n  name: test\n  log_level: info\n  api:\n    api_type: grpc\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: fake");
    EXPECT_THROW({ camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnInvalidServerAddressFormat) {
    createInvalidConfig("app:\n  name: test\n  log_level: info\n  api:\n    api_type: grpc\n    server_address: invalid_address\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: fake");
    EXPECT_THROW({ camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnInvalidCameraType) {
    createInvalidConfig("app:\n  name: test\n  log_level: info\n  api:\n    api_type: grpc\n    server_address: localhost:50051\n  core:\n    camera: invalid_camera\n  data:\n    camera: invalid_camera\n    device: fake");
    EXPECT_THROW({ camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnEmptyApiType) {
    createInvalidConfig("app:\n  name: test\n  log_level: info\n  api:\n    api_type: \n    server_address: localhost:50051\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: fake");
    EXPECT_THROW({ camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnEmptyCameraType) {
    createInvalidConfig("app:\n  name: test\n  log_level: info\n  api:\n    api_type: grpc\n    server_address: localhost:50051\n  core:\n    camera: \n  data:\n    camera: \n    device: fake");
    EXPECT_THROW({ camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnEmptyDeviceType) {
    createInvalidConfig("app:\n  name: test\n  log_level: info\n  api:\n    api_type: grpc\n    server_address: localhost:50051\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: \"\"");
    EXPECT_THROW({
        camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, HandlesAppName) {
    createInvalidConfig("app:\n  name: demo\n  log_level: info\n  api:\n    api_type: grpc\n    server_address: localhost:50051\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: fake");
    const camera_service::common::ConfigManager config(invalid_config_path_);

    const auto& name = config.getAppName();
    EXPECT_EQ(name, "demo");
}

TEST_F(ConfigManagerTests, HandlesLogLevel) {
    createInvalidConfig("app:\n  name: test\n  log_level: info\n  api:\n    api_type: grpc\n    server_address: localhost:50051\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: fake");
    const camera_service::common::ConfigManager config(invalid_config_path_);

    const auto& log_level = config.getLogLevel();
    EXPECT_EQ(log_level, "info");
}

TEST_F(ConfigManagerTests, ValidatesCoreCamera) {
    createInvalidConfig("app:\n  name: test\n  log_level: info\n  api:\n    api_type: grpc\n    server_address: localhost:50051\n  core:\n    camera: wfov\n  data:\n    camera: sony\n    device: fake");
    const camera_service::common::ConfigManager config(invalid_config_path_);

    const auto& core_config = config.getCoreConfig();
    EXPECT_EQ(core_config.camera, "wfov");
}

TEST_F(ConfigManagerTests, ThrowsOnMissingAppName) {
    createInvalidConfig("app:\n  log_level: info\n  api:\n    api_type: grpc\n    server_address: localhost:50051\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: fake");
    EXPECT_THROW({
        camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnMissingLogLevel) {
    createInvalidConfig("app:\n  name: test\n  api:\n    api_type: grpc\n    server_address: localhost:50051\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: fake");
    EXPECT_THROW({
        camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}

TEST_F(ConfigManagerTests, ThrowsOnInvalidLogLevel) {
    createInvalidConfig("app:\n  name: test\n  log_level: invalid_level\n  api:\n    api_type: grpc\n    server_address: localhost:50051\n  core:\n    camera: nfov\n  data:\n    camera: sony\n    device: fake");
    EXPECT_THROW({
        camera_service::common::ConfigManager config(invalid_config_path_);
    }, camera_service::common::ConfigException);
}
