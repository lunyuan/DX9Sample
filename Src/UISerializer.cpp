#include "UISerializer.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

json UISerializer::SerializeComponent(const UIComponentNew* component) {
    if (!component) return json();
    
    json j;
    j["id"] = component->id;
    j["name"] = std::string(component->name.begin(), component->name.end());
    j["relativeX"] = component->relativeX;
    j["relativeY"] = component->relativeY;
    j["width"] = component->width;
    j["height"] = component->height;
    j["visible"] = component->visible;
    j["enabled"] = component->enabled;
    j["dragMode"] = static_cast<int>(component->dragMode);
    
    // 根據類型序列化特定屬性
    if (const auto* image = dynamic_cast<const UIImageNew*>(component)) {
        j["componentType"] = "UIImageNew";
        j["imagePath"] = std::string(image->imagePath.begin(), image->imagePath.end());
        j["color"] = image->color;
        j["useTransparency"] = image->useTransparency;
        j["dragMode"] = static_cast<int>(image->dragMode);
        j["allowDragFromTransparent"] = image->allowDragFromTransparent;
    }
    else if (const auto* button = dynamic_cast<const UIButtonNew*>(component)) {
        j["componentType"] = "UIButtonNew";
        j["text"] = std::string(button->text.begin(), button->text.end());
        j["normalImage"] = std::string(button->normalImage.begin(), button->normalImage.end());
        j["hoverImage"] = std::string(button->hoverImage.begin(), button->hoverImage.end());
        j["pressedImage"] = std::string(button->pressedImage.begin(), button->pressedImage.end());
        j["disabledImage"] = std::string(button->disabledImage.begin(), button->disabledImage.end());
        j["textColor"] = button->textColor;
        j["backgroundColor"] = button->backgroundColor;
    }
    else if (const auto* edit = dynamic_cast<const UIEditNew*>(component)) {
        j["componentType"] = "UIEditNew";
        j["text"] = std::string(edit->text.begin(), edit->text.end());
        j["backgroundImage"] = std::string(edit->backgroundImage.begin(), edit->backgroundImage.end());
        j["textColor"] = edit->textColor;
        j["backgroundColor"] = edit->backgroundColor;
        j["borderColor"] = edit->borderColor;
        j["maxLength"] = edit->maxLength;
    }
    
    // 序列化子元件
    if (!component->children.empty()) {
        json children = json::array();
        for (const auto& child : component->children) {
            if (child) {
                children.push_back(SerializeComponent(child.get()));
            }
        }
        j["children"] = children;
    }
    
    return j;
}

json UISerializer::SerializeUISystem(const IUIManager* uiManager) {
    json j;
    j["version"] = "1.0";
    j["type"] = "UISystem";
    
    // 序列化層級資訊
    json layers = json::array();
    // 注意：UIManager的內部結構可能需要擴展以支援層級導出
    
    // 序列化所有根元件
    json rootComponents = json::array();
    const auto& components = uiManager->GetRootComponents();
    for (const auto& component : components) {
        if (component) {
            rootComponents.push_back(SerializeComponent(component.get()));
        }
    }
    
    j["layers"] = layers;
    j["components"] = rootComponents;
    
    return j;
}

std::unique_ptr<UIComponentNew> UISerializer::DeserializeComponent(const json& j, IUIManager* uiManager) {
    if (!j.contains("componentType")) return nullptr;
    
    std::string componentType = j["componentType"];
    std::unique_ptr<UIComponentNew> component;
    
    if (componentType == "UIImageNew") {
        auto image = std::make_unique<UIImageNew>();
        std::string imagePath = j.value("imagePath", "");
        image->imagePath = std::wstring(imagePath.begin(), imagePath.end());
        image->color = j.value("color", 0xFFFFFFFF);
        image->useTransparency = j.value("useTransparency", true);
        image->dragMode = static_cast<DragMode>(j.value("dragMode", 0));
        image->allowDragFromTransparent = j.value("allowDragFromTransparent", false);
        component = std::move(image);
    }
    else if (componentType == "UIButtonNew") {
        auto button = std::make_unique<UIButtonNew>();
        std::string text = j.value("text", "");
        button->text = std::wstring(text.begin(), text.end());
        
        // Debug output
        std::cout << "Creating button: " << text 
                  << " at (" << j.value("relativeX", 0) 
                  << ", " << j.value("relativeY", 0) << ")" 
                  << " visible: " << j.value("visible", true) << std::endl;
        
        std::string normalImage = j.value("normalImage", "");
        button->normalImage = std::wstring(normalImage.begin(), normalImage.end());
        
        std::string hoverImage = j.value("hoverImage", "");
        button->hoverImage = std::wstring(hoverImage.begin(), hoverImage.end());
        
        std::string pressedImage = j.value("pressedImage", "");
        button->pressedImage = std::wstring(pressedImage.begin(), pressedImage.end());
        
        std::string disabledImage = j.value("disabledImage", "");
        button->disabledImage = std::wstring(disabledImage.begin(), disabledImage.end());
        
        button->textColor = j.value("textColor", 0xFF000000);
        button->backgroundColor = j.value("backgroundColor", 0xFFC0C0C0);
        component = std::move(button);
    }
    else if (componentType == "UIEditNew") {
        auto edit = std::make_unique<UIEditNew>();
        std::string text = j.value("text", "");
        edit->text = std::wstring(text.begin(), text.end());
        
        std::string backgroundImage = j.value("backgroundImage", "");
        edit->backgroundImage = std::wstring(backgroundImage.begin(), backgroundImage.end());
        
        edit->textColor = j.value("textColor", 0xFF000000);
        edit->backgroundColor = j.value("backgroundColor", 0xFFFFFFFF);
        edit->borderColor = j.value("borderColor", 0xFF808080);
        edit->maxLength = j.value("maxLength", 256);
        component = std::move(edit);
    }
    
    if (component) {
        // 設置基本屬性
        component->id = j.value("id", 0);
        std::string name = j.value("name", "");
        component->name = std::wstring(name.begin(), name.end());
        component->relativeX = j.value("relativeX", 0);
        component->relativeY = j.value("relativeY", 0);
        component->width = j.value("width", 100);
        component->height = j.value("height", 100);
        component->visible = j.value("visible", true);
        component->enabled = j.value("enabled", true);
        component->dragMode = static_cast<DragMode>(j.value("dragMode", 0));
        // UIManager需要dynamic_cast
        component->manager = dynamic_cast<UIManager*>(uiManager);
        
        // 遞迴反序列化子元件
        if (j.contains("children") && j["children"].is_array()) {
            for (const auto& childJson : j["children"]) {
                auto child = DeserializeComponent(childJson, uiManager);
                if (child) {
                    child->parent = component.get();
                    component->children.push_back(std::move(child));
                }
            }
        }
    }
    
    return component;
}

bool UISerializer::SaveToFile(const IUIManager* uiManager, const std::filesystem::path& filepath) {
    try {
        // 使用SerializeUISystem來序列化整個UI系統
        json j = SerializeUISystem(uiManager);
        
        // 寫入檔案
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            return false;
        }
        
        file << j.dump(4); // 使用4個空格縮排
        file.close();
        
        std::cout << "UI layout saved to: " << filepath << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving UI layout: " << e.what() << std::endl;
        return false;
    }
}

bool UISerializer::LoadFromFile(IUIManager* uiManager, const std::filesystem::path& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filepath << std::endl;
            return false;
        }
        
        json j;
        file >> j;
        file.close();
        
        // 使用LoadUISystem來載入UI系統
        return LoadUISystem(uiManager, j);
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading UI layout: " << e.what() << std::endl;
        return false;
    }
}

bool UISerializer::LoadUISystem(IUIManager* uiManager, const json& j) {
    try {
        // 驗證檔案格式
        if (j.value("type", "") != "UISystem") {
            std::cerr << "Invalid UI system file format" << std::endl;
            return false;
        }
        
        // 檢查版本
        std::string version = j.value("version", "");
        if (version != "1.0") {
            std::cerr << "Unsupported UI file version: " << version << std::endl;
            return false;
        }
        
        // 清除現有UI
        uiManager->ClearAll();
        
        // 載入元件
        if (j.contains("components") && j["components"].is_array()) {
            for (const auto& componentJson : j["components"]) {
                auto component = DeserializeComponent(componentJson, uiManager);
                if (component) {
                    // 將元件添加到UIManager
                    uiManager->AddComponent(std::move(component));
                }
            }
        }
        
        std::cout << "UI system loaded successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading UI system: " << e.what() << std::endl;
        return false;
    }
}