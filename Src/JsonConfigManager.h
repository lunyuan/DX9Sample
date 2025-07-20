#pragma once

#include "IConfigManager.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

class JsonConfigManager : public IConfigManager {
public:
    JsonConfigManager();
    ~JsonConfigManager() = default;
    
    // IConfigManager 介面實作
    bool LoadConfig(const std::string& configPath) override;
    bool SaveConfig(const std::string& configPath = "") override;
    
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const override;
    int GetInt(const std::string& key, int defaultValue = 0) const override;
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const override;
    bool GetBool(const std::string& key, bool defaultValue = false) const override;
    
    std::vector<std::string> GetStringArray(const std::string& key) const override;
    std::vector<int> GetIntArray(const std::string& key) const override;
    std::vector<float> GetFloatArray(const std::string& key) const override;
    
    void SetString(const std::string& key, const std::string& value) override;
    void SetInt(const std::string& key, int value) override;
    void SetFloat(const std::string& key, float value) override;
    void SetBool(const std::string& key, bool value) override;
    
    bool HasKey(const std::string& key) const override;
    bool RemoveKey(const std::string& key) override;
    std::vector<std::string> GetAllKeys() const override;
    
    void PrintAll() const override;

private:
    // 輔助方法
    json* FindValue(const std::string& key);
    const json* FindValue(const std::string& key) const;
    void SetValue(const std::string& key, const json& value);
    std::vector<std::string> SplitKey(const std::string& key) const;
    
private:
    json config_;
    std::string configPath_;
    bool modified_;
};