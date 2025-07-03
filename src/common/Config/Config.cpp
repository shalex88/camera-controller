#include "Config.h"

#include <yaml-cpp/yaml.h>

Config::Config(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        throw ConfigException("File does not exist: " + filename);
    }
    loadFromFile(filename);
}

void Config::loadFromFile(const std::filesystem::path& filename) {
    try {
        for (YAML::Node config = YAML::LoadFile(filename); const auto& it : config) {
            auto key = it.first.as<std::string>();
            const auto value = it.second.as<std::string>();
            data_[key] = value;
        }
    } catch (const YAML::Exception& e) {
        throw ConfigException("YAML parsing error: " + std::string(e.what()));
    }
}

std::string Config::get(const std::string& key) const {
    const auto it = data_.find(key);
    if (it == data_.end()) {
        throw ConfigException("Key not found: " + key);
    }
    return it->second;
}

bool Config::has(const std::string& key) const {
    return data_.contains(key);
}

void Config::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}