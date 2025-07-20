#pragma once

#include <string>
#include <vector>
#include <memory>

// 配置管理器介面
struct IConfigManager {
    virtual ~IConfigManager() = default;
    
    // 載入配置檔案
    virtual bool LoadConfig(const std::string& configPath) = 0;
    virtual bool SaveConfig(const std::string& configPath = "") = 0;
    
    // 基本類型讀取
    virtual std::string GetString(const std::string& key, const std::string& defaultValue = "") const = 0;
    virtual int GetInt(const std::string& key, int defaultValue = 0) const = 0;
    virtual float GetFloat(const std::string& key, float defaultValue = 0.0f) const = 0;
    virtual bool GetBool(const std::string& key, bool defaultValue = false) const = 0;
    
    // 陣列類型讀取
    virtual std::vector<std::string> GetStringArray(const std::string& key) const = 0;
    virtual std::vector<int> GetIntArray(const std::string& key) const = 0;
    virtual std::vector<float> GetFloatArray(const std::string& key) const = 0;
    
    // 設置值
    virtual void SetString(const std::string& key, const std::string& value) = 0;
    virtual void SetInt(const std::string& key, int value) = 0;
    virtual void SetFloat(const std::string& key, float value) = 0;
    virtual void SetBool(const std::string& key, bool value) = 0;
    
    // 檢查鍵值是否存在
    virtual bool HasKey(const std::string& key) const = 0;
    
    // 刪除鍵值
    virtual bool RemoveKey(const std::string& key) = 0;
    
    // 獲取所有鍵值
    virtual std::vector<std::string> GetAllKeys() const = 0;
    
    // 除錯
    virtual void PrintAll() const = 0;
};

// Factory 函式
std::unique_ptr<IConfigManager> CreateConfigManager();