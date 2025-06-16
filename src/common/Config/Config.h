#pragma once
#include <string>
#include <unordered_map>
#include <stdexcept>

class ConfigException final : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message) : std::runtime_error(message) {}
};

class Config {
public:
    Config() = delete;
    explicit Config(const std::string& filename);
    void loadFromFile(const std::string& filename);
    std::string get(const std::string& key) const;
    bool has(const std::string& key) const;
    void set(const std::string& key, const std::string& value);
private:
    std::unordered_map<std::string, std::string> data_;
};
