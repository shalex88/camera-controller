#pragma once
#include <string>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <yaml-cpp/yaml.h>

class ConfigException final : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message) : std::runtime_error(message) {}
};

struct ApiConfig {
    std::string api;
    std::string server_address;

    void validate() const;
};

struct CoreConfig {
    std::string camera;

    void validate() const;
};

struct DataConfig {
    std::string camera;
    std::string device;

    void validate() const;
};

struct AppConfig {
    ApiConfig api_config;
    CoreConfig core_config;
    DataConfig data_config;
    std::string log_level;
    std::string name;

    void validate() const;
};

class ConfigManager {
public:
    ConfigManager() = delete;
    explicit ConfigManager(const std::string& filename);

    const ApiConfig& getApiConfig() const;
    const CoreConfig& getCoreConfig() const;
    const DataConfig& getDataConfig() const;
    const std::string& getLogLevel() const;
    const std::string& getAppName() const;

private:
    void loadFromFile(const std::filesystem::path& filename) const;
    void validateConfiguration() const;
    void loadApiConfig(const YAML::Node& app_node) const;
    void loadCoreConfig(const YAML::Node& app_node) const;
    void loadDataConfig(const YAML::Node& app_node) const;
    void loadAppConfig(const YAML::Node& app_node) const;

    std::unique_ptr<AppConfig> app_config_;
};
