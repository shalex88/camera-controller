#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* Add your project include files here */
#include "common/Config/Config.h"

#include <filesystem>
#include <fstream>

using namespace testing;

class ConfigTests : public Test {
protected:
    const std::string test_config_path_ = "test_config.yaml";

    void SetUp() override {
        std::ofstream config_file(test_config_path_);
        config_file << "api: grpc\n";
        config_file << "camera: nfov\n";
        config_file.close();
    }

    void TearDown() override {
        if (std::filesystem::exists(test_config_path_)) {
            std::filesystem::remove(test_config_path_);
        }
    }
};

TEST_F(ConfigTests, LoadValidConfig) {
    const Config config(test_config_path_);
    EXPECT_EQ(config.get("api"), "grpc");
    EXPECT_EQ(config.get("camera"), "nfov");
}

TEST_F(ConfigTests, ThrowsOnNonexistentFile) {
    EXPECT_THROW(Config("nonexistent.yaml"), ConfigException);
}

TEST_F(ConfigTests, ThrowsOnInvalidYaml) {
    std::ofstream config_file("invalid.yaml");
    config_file << "invalid: : yaml : content";
    config_file.close();

    EXPECT_THROW(Config("invalid.yaml"), ConfigException);
    std::filesystem::remove("invalid.yaml");
}

TEST_F(ConfigTests, GetNonexistentKey) {
    const Config config(test_config_path_);
    EXPECT_THROW(config.get("nonexistent"), ConfigException);
}

TEST_F(ConfigTests, HasKey) {
    const Config config(test_config_path_);
    ASSERT_TRUE(config.has("camera"));
    EXPECT_FALSE(config.has("nonexistent"));
}

TEST_F(ConfigTests, SetAndGetValue) {
    Config config(test_config_path_);
    config.set("new_key", "new_value");
    ASSERT_TRUE(config.has("new_key"));
    EXPECT_EQ(config.get("new_key"), "new_value");
}

TEST_F(ConfigTests, OverwriteExistingValue) {
    Config config(test_config_path_);
    config.set("camera", "wfov");
    EXPECT_EQ(config.get("camera"), "wfov");
}
