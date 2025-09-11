#include "ConfigManager.h"

#include <yaml-cpp/yaml.h>
#include <set>

void ApiConfig::validate() const {
    static const std::set<std::string> valid_apis{"grpc"};

    if (api.empty()) {
        throw ConfigException("API type cannot be empty");
    }
    if (!valid_apis.contains(api)) {
        throw ConfigException("Invalid API type: " + api);
    }
    if (server_address.empty()) {
        throw ConfigException("Server address cannot be empty");
    }
    if (server_address.find(':') == std::string::npos) {
        throw ConfigException("Server address must include port (format: host:port)");
    }
}

void CoreConfig::validate() const {
    static const std::set<std::string> valid_cameras{"nfov", "wfov"};

    if (camera.empty()) {
        throw ConfigException("Camera type cannot be empty");
    }
}

void DataConfig::validate() const {
    static const std::set<std::string> valid_cameras{"sony", "adimec", "sony-visca"};

    if (camera.empty()) {
        throw ConfigException("Data camera type cannot be empty");
    }
    if (!valid_cameras.contains(camera)) {
        throw ConfigException("Invalid data camera type: " + camera);
    }
    if (device.empty()) {
        throw ConfigException("Device type cannot be empty");
    }
}

void AppConfig::validate() const {
    static const std::set<std::string> valid_log_levels{"trace", "debug", "info", "warn", "error", "critical"};

    api_config.validate();
    core_config.validate();
    data_config.validate();

    if (log_level.empty()) {
        throw ConfigException("Log level cannot be empty");
    }
    if (!valid_log_levels.contains(log_level)) {
        throw ConfigException("Invalid log level: " + log_level);
    }
    if (name.empty()) {
        throw ConfigException("App name cannot be empty");
    }

    // Cross-validation: ensure camera types are consistent
    if (api_config.server_address == core_config.camera) {
        throw ConfigException("API server address cannot be the same as camera type");
    }
}

ConfigManager::ConfigManager(const std::string& filename)
    : app_config_(std::make_unique<AppConfig>()) {
    if (!std::filesystem::exists(filename)) {
        throw ConfigException("Configuration file does not exist: " + filename);
    }
    loadFromFile(filename);
    validateConfiguration();
}

void ConfigManager::loadFromFile(const std::filesystem::path& filename) const {
    try {
        if (const YAML::Node config = YAML::LoadFile(filename); config["app"]) {
            const auto& app_node = config["app"];
            loadApiConfig(app_node);
            loadCoreConfig(app_node);
            loadDataConfig(app_node);
            loadAppConfig(app_node);
        }
    } catch (const YAML::Exception& e) {
        throw ConfigException("YAML parsing error: " + std::string(e.what()));
    }
}

void ConfigManager::loadApiConfig(const YAML::Node& app_node) const {
    if (app_node["api"]) {
        const auto& api_node = app_node["api"];
        if (api_node["api_type"]) {
            app_config_->api_config.api = api_node["api_type"].as<std::string>();
        }
        if (api_node["server_address"]) {
            app_config_->api_config.server_address = api_node["server_address"].as<std::string>();
        }
    }
}

void ConfigManager::loadCoreConfig(const YAML::Node& app_node) const {
    if (app_node["core"]) {
        const auto& core_node = app_node["core"];
        if (core_node["camera"]) {
            app_config_->core_config.camera = core_node["camera"].as<std::string>();
        }
    }
}

void ConfigManager::loadDataConfig(const YAML::Node& app_node) const {
    if (app_node["data"]) {
        const auto& data_node = app_node["data"];
        if (data_node["camera"]) {
            app_config_->data_config.camera = data_node["camera"].as<std::string>();
        }
        if (data_node["device"]) {
            app_config_->data_config.device = data_node["device"].as<std::string>();
        }
    }
}

void ConfigManager::loadAppConfig(const YAML::Node& app_node) const {
    if (app_node["log_level"]) {
        app_config_->log_level = app_node["log_level"].as<std::string>();
    }
    if (app_node["name"]) {
        app_config_->name = app_node["name"].as<std::string>();
    }
}

const ApiConfig& ConfigManager::getApiConfig() const {
    return app_config_->api_config;
}

const CoreConfig& ConfigManager::getCoreConfig() const {
    return app_config_->core_config;
}

const DataConfig& ConfigManager::getDataConfig() const {
    return app_config_->data_config;
}

const std::string& ConfigManager::getLogLevel() const {
    return app_config_->log_level;
}

const std::string& ConfigManager::getAppName() const {
    return app_config_->name;
}

void ConfigManager::validateConfiguration() const {
    if (!app_config_) {
        throw ConfigException("Configuration not initialized");
    }
    app_config_->validate();
}