#include "ServiceLocator.h"
#include <iostream>

ServiceLocator::ServiceLocator()
    : assetManager_(nullptr)
    , uiManager_(nullptr)
    , eventManager_(nullptr)
    , configManager_(nullptr)
    , device_(nullptr)
{
}

bool ServiceLocator::ValidateServices() const {
    bool valid = true;
    
    if (!assetManager_) {
        std::cerr << "ServiceLocator: AssetManager not set" << std::endl;
        valid = false;
    }
    
    if (!uiManager_) {
        std::cerr << "ServiceLocator: UIManager not set" << std::endl;
        valid = false;
    }
    
    if (!device_) {
        std::cerr << "ServiceLocator: D3D Device not set" << std::endl;
        valid = false;
    }
    
    // EventManager 和 ConfigManager 是可選的
    if (!eventManager_) {
        std::cout << "ServiceLocator: Warning - EventManager not set" << std::endl;
    }
    
    if (!configManager_) {
        std::cout << "ServiceLocator: Warning - ConfigManager not set" << std::endl;
    }
    
    return valid;
}

void ServiceLocator::PrintServiceStatus() const {
    std::cout << "\n=== Service Locator Status ===" << std::endl;
    std::cout << "AssetManager: " << (assetManager_ ? "SET" : "NOT SET") << std::endl;
    std::cout << "UIManager: " << (uiManager_ ? "SET" : "NOT SET") << std::endl;
    std::cout << "EventManager: " << (eventManager_ ? "SET" : "NOT SET") << std::endl;
    std::cout << "ConfigManager: " << (configManager_ ? "SET" : "NOT SET") << std::endl;
    std::cout << "D3D Device: " << (device_ ? "SET" : "NOT SET") << std::endl;
    std::cout << "==============================\n" << std::endl;
}