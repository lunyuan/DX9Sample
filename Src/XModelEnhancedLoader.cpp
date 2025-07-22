#include "XModelEnhancedLoader.h"
#include <iostream>

std::map<std::string, ModelData> XModelEnhancedLoader::Load(
    const std::filesystem::path& file, 
    IDirect3DDevice9* device) const {
    
    std::map<std::string, ModelData> result;
    
    try {
        // Use XModelEnhanced to load with separation
        auto models = XModelEnhanced::LoadWithSeparation(file, device);
        
        // Convert shared_ptr<ModelData> to ModelData
        for (const auto& [name, modelPtr] : models) {
            if (modelPtr) {
                result[name] = *modelPtr;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "XModelEnhancedLoader: Failed to load " << file << ": " << e.what() << std::endl;
    }
    
    return result;
}

std::vector<std::string> XModelEnhancedLoader::GetModelNames(
    const std::filesystem::path& file) const {
    
    // For now, we need to actually load the file to get names
    // This could be optimized later to just parse the file structure
    std::vector<std::string> names;
    
    try {
        // Create a temporary device for loading
        // Note: This is not ideal, but GetObjectNames requires a device
        // In a real implementation, we might want to parse the file directly
        IDirect3DDevice9* device = nullptr;
        // TODO: Get device from somewhere or parse file without loading
        
        names = XModelEnhanced::GetObjectNames(file, device);
    }
    catch (const std::exception& e) {
        std::cerr << "XModelEnhancedLoader: Failed to get names from " << file << ": " << e.what() << std::endl;
    }
    
    return names;
}