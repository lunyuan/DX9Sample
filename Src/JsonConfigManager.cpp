#include "JsonConfigManager.h"
#include <iostream>
#include <algorithm>
#include <sstream>

// Factory 函式實作
std::unique_ptr<IConfigManager> CreateConfigManager() {
    return std::make_unique<JsonConfigManager>();
}

JsonConfigManager::JsonConfigManager() : modified_(false) {
    // 建立預設配置
    config_ = json::object();
}

bool JsonConfigManager::LoadConfig(const std::string& configPath) {
    try {
        configPath_ = configPath;
        
        std::ifstream file(configPath);
        if (!file.is_open()) {
            std::cerr << "Warning: Config file not found: " << configPath << std::endl;
            std::cerr << "Creating default configuration..." << std::endl;
            
            // 建立預設配置
            config_ = {
                {"engine", {
                    {"name", "DX9Sample Engine"},
                    {"version", "1.0.0"}
                }},
                {"assets", {
                    {"rootPath", "test/"},
                    {"modelPath", "models/"},
                    {"texturePath", "textures/"},
                    {"soundPath", "sounds/"},
                    {"scriptPath", "scripts/"},
                    {"configPath", "configs/"}
                }},
                {"graphics", {
                    {"width", 800},
                    {"height", 600},
                    {"fullscreen", false},
                    {"vsync", true}
                }},
                {"scenes", {
                    {"defaultScene", "GameScene"},
                    {"menuScene", "MenuScene"}
                }},
                {"ui", {
                    {"persistentLayers", json::array({"HUD", "Debug"})},
                    {"theme", "default"}
                }},
                {"debug", {
                    {"enableLogging", true},
                    {"logLevel", "info"},
                    {"showFPS", true},
                    {"enableHotReload", false}
                }}
            };
            
            // 保存預設配置
            SaveConfig(configPath);
            return true;
        }
        
        file >> config_;
        file.close();
        
        modified_ = false;
        return true;
    }
    catch (const json::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        return false;
    }
}

bool JsonConfigManager::SaveConfig(const std::string& configPath) {
    try {
        std::string path = configPath.empty() ? configPath_ : configPath;
        if (path.empty()) {
            std::cerr << "No config path specified" << std::endl;
            return false;
        }
        
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << path << std::endl;
            return false;
        }
        
        file << config_.dump(4); // 4 spaces indentation
        file.close();
        
        modified_ = false;
        return true;
    }
    catch (const json::exception& e) {
        std::cerr << "Failed to save config: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> JsonConfigManager::SplitKey(const std::string& key) const {
    std::vector<std::string> tokens;
    std::stringstream ss(key);
    std::string token;
    
    while (std::getline(ss, token, '.')) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

json* JsonConfigManager::FindValue(const std::string& key) {
    auto tokens = SplitKey(key);
    if (tokens.empty()) return nullptr;
    
    json* current = &config_;
    for (const auto& token : tokens) {
        if (current->is_object() && current->contains(token)) {
            current = &(*current)[token];
        } else {
            return nullptr;
        }
    }
    
    return current;
}

const json* JsonConfigManager::FindValue(const std::string& key) const {
    auto tokens = SplitKey(key);
    if (tokens.empty()) return nullptr;
    
    const json* current = &config_;
    for (const auto& token : tokens) {
        if (current->is_object() && current->contains(token)) {
            current = &(*current)[token];
        } else {
            return nullptr;
        }
    }
    
    return current;
}

void JsonConfigManager::SetValue(const std::string& key, const json& value) {
    auto tokens = SplitKey(key);
    if (tokens.empty()) return;
    
    json* current = &config_;
    for (size_t i = 0; i < tokens.size() - 1; ++i) {
        const std::string& token = tokens[i];
        if (!current->is_object()) {
            *current = json::object();
        }
        if (!current->contains(token)) {
            (*current)[token] = json::object();
        }
        current = &(*current)[token];
    }
    
    if (!current->is_object()) {
        *current = json::object();
    }
    
    (*current)[tokens.back()] = value;
    modified_ = true;
}

std::string JsonConfigManager::GetString(const std::string& key, const std::string& defaultValue) const {
    const json* value = FindValue(key);
    if (value && value->is_string()) {
        return value->get<std::string>();
    }
    return defaultValue;
}

int JsonConfigManager::GetInt(const std::string& key, int defaultValue) const {
    const json* value = FindValue(key);
    if (value && value->is_number_integer()) {
        return value->get<int>();
    }
    return defaultValue;
}

float JsonConfigManager::GetFloat(const std::string& key, float defaultValue) const {
    const json* value = FindValue(key);
    if (value && value->is_number()) {
        return value->get<float>();
    }
    return defaultValue;
}

bool JsonConfigManager::GetBool(const std::string& key, bool defaultValue) const {
    const json* value = FindValue(key);
    if (value && value->is_boolean()) {
        return value->get<bool>();
    }
    return defaultValue;
}

std::vector<std::string> JsonConfigManager::GetStringArray(const std::string& key) const {
    const json* value = FindValue(key);
    if (value && value->is_array()) {
        std::vector<std::string> result;
        for (const auto& item : *value) {
            if (item.is_string()) {
                result.push_back(item.get<std::string>());
            }
        }
        return result;
    }
    return {};
}

std::vector<int> JsonConfigManager::GetIntArray(const std::string& key) const {
    const json* value = FindValue(key);
    if (value && value->is_array()) {
        std::vector<int> result;
        for (const auto& item : *value) {
            if (item.is_number_integer()) {
                result.push_back(item.get<int>());
            }
        }
        return result;
    }
    return {};
}

std::vector<float> JsonConfigManager::GetFloatArray(const std::string& key) const {
    const json* value = FindValue(key);
    if (value && value->is_array()) {
        std::vector<float> result;
        for (const auto& item : *value) {
            if (item.is_number()) {
                result.push_back(item.get<float>());
            }
        }
        return result;
    }
    return {};
}

void JsonConfigManager::SetString(const std::string& key, const std::string& value) {
    SetValue(key, json(value));
}

void JsonConfigManager::SetInt(const std::string& key, int value) {
    SetValue(key, json(value));
}

void JsonConfigManager::SetFloat(const std::string& key, float value) {
    SetValue(key, json(value));
}

void JsonConfigManager::SetBool(const std::string& key, bool value) {
    SetValue(key, json(value));
}

bool JsonConfigManager::HasKey(const std::string& key) const {
    return FindValue(key) != nullptr;
}

bool JsonConfigManager::RemoveKey(const std::string& key) {
    auto tokens = SplitKey(key);
    if (tokens.empty()) return false;
    
    json* current = &config_;
    for (size_t i = 0; i < tokens.size() - 1; ++i) {
        const std::string& token = tokens[i];
        if (current->is_object() && current->contains(token)) {
            current = &(*current)[token];
        } else {
            return false;
        }
    }
    
    if (current->is_object() && current->contains(tokens.back())) {
        current->erase(tokens.back());
        modified_ = true;
        return true;
    }
    
    return false;
}

std::vector<std::string> JsonConfigManager::GetAllKeys() const {
    std::vector<std::string> keys;
    
    std::function<void(const json&, const std::string&)> collectKeys = 
        [&](const json& obj, const std::string& prefix) {
            if (obj.is_object()) {
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    std::string fullKey = prefix.empty() ? it.key() : prefix + "." + it.key();
                    if (it.value().is_object()) {
                        collectKeys(it.value(), fullKey);
                    } else {
                        keys.push_back(fullKey);
                    }
                }
            }
        };
    
    collectKeys(config_, "");
    return keys;
}

void JsonConfigManager::PrintAll() const {
    // Configuration debug output removed for minimal logging
}