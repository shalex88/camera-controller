#include "Config.h"

#include <filesystem>

#include <yaml-cpp/yaml.h>

Config::Config(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        throw ConfigException("File does not exist: " + filename);
    }
    load_from_file(filename);
}

void Config::load_from_file(const std::string& filename) {
    try {
        YAML::Node config = YAML::LoadFile(filename);
        
        // Convert YAML nodes to string key-value pairs
        for (const auto& it : config) {
            std::string key = it.first.as<std::string>();
            std::string value = it.second.as<std::string>();
            data_[key] = value;
        }
    } catch (const YAML::Exception& e) {
        throw ConfigException("YAML parsing error: " + std::string(e.what()));
    }
}

std::string Config::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        throw ConfigException("Key not found: " + key);
    }
    return it->second;
}

bool Config::has(const std::string& key) const {
    return data_.find(key) != data_.end();
}

void Config::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}