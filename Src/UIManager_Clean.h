#pragma once
#include "IUIManager.h"
#include "ITextureManager.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <wrl/client.h>
#include <d3dx9.h>
#include <shared_mutex>

using Microsoft::WRL::ComPtr;

// Forward declarations
struct UIComponent;
class UIManager;

// Base UI component class
struct UIComponent {
    int id;
    std::wstring name;
    int relativeX, relativeY;
    int width, height;
    bool visible = true;
    bool enabled = true;
    
    UIComponent* parent = nullptr;
    std::vector<std::unique_ptr<UIComponent>> children;
    UIManager* manager = nullptr;
    
    // Calculate absolute coordinates
    virtual RECT GetAbsoluteRect() const {
        int absX = relativeX;
        int absY = relativeY;
        if (parent) {
            RECT parentRect = parent->GetAbsoluteRect();
            absX += parentRect.left;
            absY += parentRect.top;
        }
        return {absX, absY, absX + width, absY + height};
    }
    
    // Event handling - only called when mouse is over this component
    virtual bool OnMouseMove(int x, int y) { return false; }
    virtual bool OnMouseDown(int x, int y, bool isRightButton) { return false; }
    virtual bool OnMouseUp(int x, int y, bool isRightButton) { return false; }
    virtual bool OnKeyDown(WPARAM key) { return false; }
    virtual bool OnChar(WPARAM ch) { return false; }
    
    virtual void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) = 0;
    
    virtual ~UIComponent() = default;
};

// Image component
struct UIImage : public UIComponent {
    std::wstring imagePath;
    D3DCOLOR color = 0xFFFFFFFF;
    bool useTransparency = true;
    bool draggable = false;
    bool allowDragFromTransparent = false;
    
    void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) override;
    bool OnMouseDown(int x, int y, bool isRightButton) override;
};

// Button component with four-state support
struct UIButton : public UIComponent {
    std::wstring text;
    
    // Four-state images (optional)
    std::wstring normalImage;
    std::wstring hoverImage;
    std::wstring pressedImage;
    std::wstring disabledImage;
    
    // State
    enum class State { Normal, Hover, Pressed, Disabled } state = State::Normal;
    
    // Colors (used when no images provided)
    D3DCOLOR textColor = 0xFF000000;
    D3DCOLOR backgroundColor = 0xFFC0C0C0;
    
    std::function<void()> onClick;
    
    void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) override;
    bool OnMouseMove(int x, int y) override;
    bool OnMouseDown(int x, int y, bool isRightButton) override;
    bool OnMouseUp(int x, int y, bool isRightButton) override;
};

// Edit control with Unicode support
struct UIEdit : public UIComponent {
    std::wstring text;
    std::wstring backgroundImage;
    D3DCOLOR textColor = 0xFF000000;
    D3DCOLOR backgroundColor = 0xFFFFFFFF;
    D3DCOLOR borderColor = 0xFF808080;
    
    bool isFocused = false;
    int cursorPos = 0;
    int maxLength = 256;
    
    void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) override;
    bool OnMouseDown(int x, int y, bool isRightButton) override;
    bool OnKeyDown(WPARAM key) override;
    bool OnChar(WPARAM ch) override;
};

class UIManager : public IUIManager {
public:
    UIManager(ITextureManager* textureManager = nullptr);
    
    // IUIManager interface
    HRESULT Init(IDirect3DDevice9* dev) override;
    HRESULT Render(IDirect3DDevice9* dev) override;
    bool HandleMessage(const MSG& msg) override;
    
    void RegisterUIListener(IUIInputListener* listener) override {
        uiListeners_.push_back(listener);
    }
    
    // Layer management (simplified - layers removed in clean version)
    int CreateLayer(const std::wstring& name, float priority = 0.0f, float alpha = 1.0f) override { return 0; }
    void SetLayerVisible(int layerId, bool visible) override {}
    void SetLayerAlpha(int layerId, float alpha) override {}
    
    // Legacy text system (to be replaced with UIText component)
    int AddText(const std::wstring& text, int x, int y, int width, int height, 
                unsigned long color = 0xFFFFFFFF, int layer = 0) override;
    void UpdateText(int textId, const std::wstring& newText) override;
    
    // Legacy methods (to be removed)
    int AddImage(const std::wstring& imagePath, int x, int y, int width, int height,
                 bool useTransparency = true, unsigned long color = 0xFFFFFFFF, 
                 int layer = 0, bool draggable = false) override { return -1; }
    int AddButton(const std::wstring& text, int x, int y, int width, int height,
                  std::function<void()> onClick, int layer = 0, bool draggable = false) override { return -1; }
    int AddImageButton(const std::wstring& imagePath, int x, int y, int width, int height,
                       std::function<void()> onClick, int layer = 0, bool draggable = false) override { return -1; }
    void SetButtonVisible(int buttonId, bool visible) override {}
    void SetImageVisible(int imageId, bool visible) override {}
    
    void ClearLayer(int layer) override {}
    void ClearAll() override;
    
    // Component creation
    UIComponentNew* CreateImage(const std::wstring& imagePath, int x, int y, int width, int height, 
                               bool draggable = false, UIComponentNew* parent = nullptr,
                               bool allowDragFromTransparent = false) override;
    UIComponentNew* CreateButton(const std::wstring& text, int x, int y, int width, int height,
                                std::function<void()> onClick, UIComponentNew* parent = nullptr,
                                const std::wstring& normalImage = L"",
                                const std::wstring& hoverImage = L"", 
                                const std::wstring& pressedImage = L"", 
                                const std::wstring& disabledImage = L"") override;
    UIComponentNew* CreateEdit(int x, int y, int width, int height, UIComponentNew* parent = nullptr,
                              const std::wstring& backgroundImage = L"") override;
    
    // Component lookup
    UIComponentNew* FindComponentByName(const std::wstring& name) override;
    UIComponentNew* FindComponentById(int id) override;
    
    // Template methods for type-safe lookup
    template<typename T>
    T* FindComponentByName(const std::wstring& name) {
        return dynamic_cast<T*>(FindComponentByName(name));
    }
    
    template<typename T>
    T* FindComponentById(int id) {
        return dynamic_cast<T*>(FindComponentById(id));
    }
    
    // Event listeners
    void AddUIListener(IUIListener* listener) override;
    void RemoveUIListener(IUIListener* listener) override;
    
    // Serialization support
    const std::vector<std::unique_ptr<UIComponentNew>>& GetRootComponents() const override { 
        return rootComponents_; 
    }
    void AddComponent(std::unique_ptr<UIComponentNew> component) override;
    
    // Helper methods
    bool GetImageSize(const std::wstring& imagePath, int& width, int& height) const;
    void ClearAlphaMaskCache() { alphaMaskCache_.clear(); }

private:
    // Alpha mask cache for pixel-perfect mouse detection
    struct AlphaMask {
        int width;
        int height;
        std::vector<bool> mask;
    };
    
    // Members
    ComPtr<ID3DXFont> font_;
    ComPtr<ID3DXSprite> sprite_;
    ITextureManager* textureManager_;
    
    std::vector<IUIInputListener*> uiListeners_;
    std::vector<IUIListener*> uiEventListeners_;
    std::vector<std::unique_ptr<UIComponentNew>> rootComponents_;
    
    mutable std::unordered_map<std::wstring, AlphaMask> alphaMaskCache_;
    mutable std::shared_mutex alphaCacheMutex_;
    
    UIComponent* focusedComponent_ = nullptr;
    UIComponent* hoveredComponent_ = nullptr;
    UIComponent* draggedComponent_ = nullptr;
    
    int nextId_ = 0;
    POINT lastMousePos_ = {0, 0};
    POINT dragOffset_ = {0, 0};
    bool isDragging_ = false;
    
    // Private methods
    void BuildAlphaMask(const std::wstring& imagePath) const;
    bool IsPointInTransparentArea(int x, int y, const std::wstring& imagePath, const RECT& rect);
    
    UIComponent* GetComponentAt(int x, int y);
    UIComponent* GetDraggableComponentAt(int x, int y);
    void SetFocusedComponent(UIComponent* component);
    
    void RenderComponents(IDirect3DDevice9* dev, const std::vector<std::unique_ptr<UIComponentNew>>& components);
    
    void NotifyButtonClicked(UIButton* button);
    void NotifyComponentClicked(UIComponent* component);
    
    // Temporary text rendering (to be replaced)
    struct UITextElement {
        int id;
        std::wstring text;
        RECT rect;
        D3DCOLOR color;
    };
    std::vector<UITextElement> textElements_;
};