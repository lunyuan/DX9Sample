#include "UISerializer.h"
#include <fstream>
#include <iostream>
#include <windows.h>

using json = nlohmann::json;

// Helper function to convert wide string to UTF-8
static std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    
    // Get the required buffer size
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    
    // Convert
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

// Helper function to convert UTF-8 to wide string
static std::wstring UTF8ToWString(const std::string& str) {
    if (str.empty()) return L"";
    
    // Get the required buffer size
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size <= 0) return L"";
    
    // Convert
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    return result;
}

json UISerializer::SerializeComponent(const UIComponentNew* component) {
    if (!component) return json();
    
    json j;
    j["id"] = component->id;
    j["name"] = WStringToUTF8(component->name);
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
        j["imagePath"] = WStringToUTF8(image->imagePath);
        j["color"] = image->color;
        j["useTransparency"] = image->useTransparency;
        j["dragMode"] = static_cast<int>(image->dragMode);
        j["allowDragFromTransparent"] = image->allowDragFromTransparent;
    }
    else if (const auto* button = dynamic_cast<const UIButtonNew*>(component)) {
        j["componentType"] = "UIButtonNew";
        j["text"] = WStringToUTF8(button->text);
        j["normalImage"] = WStringToUTF8(button->normalImage);
        j["hoverImage"] = WStringToUTF8(button->hoverImage);
        j["pressedImage"] = WStringToUTF8(button->pressedImage);
        j["disabledImage"] = WStringToUTF8(button->disabledImage);
        j["textColor"] = button->textColor;
        j["backgroundColor"] = button->backgroundColor;
    }
    else if (const auto* edit = dynamic_cast<const UIEditNew*>(component)) {
        j["componentType"] = "UIEditNew";
        j["text"] = WStringToUTF8(edit->text);
        j["backgroundImage"] = WStringToUTF8(edit->backgroundImage);
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
        image->imagePath = UTF8ToWString(imagePath);
        image->color = j.value("color", 0xFFFFFFFF);
        image->useTransparency = j.value("useTransparency", true);
        image->dragMode = static_cast<DragMode>(j.value("dragMode", 0));
        image->allowDragFromTransparent = j.value("allowDragFromTransparent", false);
        component = std::move(image);
    }
    else if (componentType == "UIButtonNew") {
        auto button = std::make_unique<UIButtonNew>();
        std::string text = j.value("text", "");
        button->text = UTF8ToWString(text);
        
        // Debug output
        std::cout << "Creating button: " << text 
                  << " at (" << j.value("relativeX", 0) 
                  << ", " << j.value("relativeY", 0) << ")" 
                  << " visible: " << j.value("visible", true) << std::endl;
        
        std::string normalImage = j.value("normalImage", "");
        button->normalImage = UTF8ToWString(normalImage);
        
        std::string hoverImage = j.value("hoverImage", "");
        button->hoverImage = UTF8ToWString(hoverImage);
        
        std::string pressedImage = j.value("pressedImage", "");
        button->pressedImage = UTF8ToWString(pressedImage);
        
        std::string disabledImage = j.value("disabledImage", "");
        button->disabledImage = UTF8ToWString(disabledImage);
        
        button->textColor = j.value("textColor", 0xFF000000);
        button->backgroundColor = j.value("backgroundColor", 0xFFC0C0C0);
        component = std::move(button);
    }
    else if (componentType == "UIEditNew") {
        auto edit = std::make_unique<UIEditNew>();
        std::string text = j.value("text", "");
        edit->text = UTF8ToWString(text);
        
        std::string backgroundImage = j.value("backgroundImage", "");
        edit->backgroundImage = UTF8ToWString(backgroundImage);
        
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
        component->name = UTF8ToWString(name);
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