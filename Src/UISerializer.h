#pragma once
#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include "json.hpp"
#include "IUIManager.h"
#include "UIManager.h"

class UISerializer {
public:
    // 序列化UI元件到JSON
    static nlohmann::json SerializeComponent(const UIComponentNew* component);
    
    // 序列化整個UI系統
    static nlohmann::json SerializeUISystem(const IUIManager* uiManager);
    
    // 從JSON反序列化UI元件
    static std::unique_ptr<UIComponentNew> DeserializeComponent(const nlohmann::json& json, IUIManager* uiManager);
    
    // 載入整個UI系統
    static bool LoadUISystem(IUIManager* uiManager, const nlohmann::json& json);
    
    // 儲存UI到檔案
    static bool SaveToFile(const IUIManager* uiManager, const std::filesystem::path& filepath);
    
    // 從檔案載入UI
    static bool LoadFromFile(IUIManager* uiManager, const std::filesystem::path& filepath);
    
private:
    // 序列化不同類型的元件
    static nlohmann::json SerializeImage(const UIImageNew* image);
    static nlohmann::json SerializeButton(const UIButtonNew* button);
    static nlohmann::json SerializeEdit(const UIEditNew* edit);
    
    // 反序列化不同類型的元件
    static std::unique_ptr<UIImageNew> DeserializeImage(const nlohmann::json& json);
    static std::unique_ptr<UIButtonNew> DeserializeButton(const nlohmann::json& json);
    static std::unique_ptr<UIEditNew> DeserializeEdit(const nlohmann::json& json);
    
    // 遞迴序列化子元件
    static void SerializeChildren(const UIComponentNew* component, nlohmann::json& json);
    
    // 遞迴反序列化子元件
    static void DeserializeChildren(UIComponentNew* component, const nlohmann::json& json, UIManager* uiManager);
};