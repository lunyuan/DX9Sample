#include "ServiceLocator.h"
#include <iostream>

ServiceLocator::ServiceLocator()
    : assetManager_(nullptr)
    , uiManager_(nullptr)
    // , uiSystem_(nullptr) // Removed - using UIManager only
    , eventManager_(nullptr)
    , configManager_(nullptr)
    , sceneManager_(nullptr)
    , device_(nullptr)
    , cameraController_(nullptr)
{
}

bool ServiceLocator::ValidateServices() const {
    bool valid = true;
    
    // 必要服務
    if (!assetManager_) {
        std::cerr << "ServiceLocator: AssetManager not set (required)" << std::endl;
        valid = false;
    }
    
    if (!uiManager_) {
        std::cerr << "ServiceLocator: UIManager not set (required)" << std::endl;
        valid = false;
    }
    
    if (!device_) {
        std::cerr << "ServiceLocator: D3D Device not set (required)" << std::endl;
        valid = false;
    }
    
    if (!sceneManager_) {
        std::cerr << "ServiceLocator: SceneManager not set (required)" << std::endl;
        valid = false;
    }
    
    // 可選但建議的服務
    if (!eventManager_) {
        std::cerr << "ServiceLocator: EventManager not set (optional but recommended)" << std::endl;
    }
    
    if (!configManager_) {
        std::cerr << "ServiceLocator: ConfigManager not set (optional)" << std::endl;
    }
    
    return valid;
}

void ServiceLocator::PrintServiceStatus() const {
    // Service status debug info removed for minimal logging
}