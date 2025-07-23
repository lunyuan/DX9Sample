# UI System Complete Guide

## Overview

The UI system provides a comprehensive framework for creating and managing user interfaces in the DirectX 9 engine. It features a layered architecture, drag-and-drop support, event-driven interactions, and cross-scene persistence capabilities.

### Key Features
- **Layered UI Architecture**: Organized rendering with priority-based layers
- **Drag System**: Flexible drag modes including Move, MoveRevert, and DragDrop
- **Event System**: Type-safe event handling for UI interactions
- **Cross-Scene Persistence**: UI elements that persist across scene transitions
- **Parent-Child Relationships**: Hierarchical component organization
- **Dynamic Updates**: Real-time text and property modifications

## Architecture

### Core Components

#### IUISystem Interface
The main interface for the UI system, providing:
- Layer management (creation, retrieval, removal)
- Input handling and event processing
- Rendering coordination
- Global event registration

#### UI Layers
UI elements are organized into layers with different purposes and priorities:

```cpp
enum class UILayerType {
    Background,    // Background layer (priority: 0)
    World,         // World/game layer (priority: 100)
    Interface,     // Interface layer (priority: 200)
    Overlay,       // Overlay layer - menus, dialogs (priority: 300)
    Persistent,    // Persistent layer - HUD, debug info (priority: 400)
    System         // System layer - topmost (priority: 500)
};
```

#### UI Components
Base component class hierarchy:
```cpp
UIComponentNew
├── UIButtonNew      // Clickable buttons
├── UIImageNew       // Images with drag support
├── UITextNew        // Text display
├── UIEditNew        // Text input fields
└── UISliderNew      // Value sliders
```

### Component Properties

All UI components share common properties:
- **Position**: `relativeX`, `relativeY` (relative to parent)
- **Size**: `width`, `height`
- **Visibility**: `visible` flag
- **Drag Support**: `dragMode` enum
- **Parent-Child**: `parent` pointer and `children` vector
- **Events**: Component-specific callbacks

## Drag System Implementation

### Drag Modes

```cpp
enum class DragMode {
    None,           // Not draggable
    Move,           // Moveable (stays at new position)
    MoveRevert,     // Moveable but returns to original position
    DragDrop        // Drag and drop mode (can drop onto other components)
};
```

### Drag System Architecture

#### State Variables in UIManager
```cpp
UIComponentNew* draggedComponent_ = nullptr;  // Currently dragged component
UIComponentNew* dropTarget_ = nullptr;        // Current drop target
bool isInDragDropMode_ = false;              // Drag-drop mode flag
POINT dragOffset_ = {0, 0};                  // Mouse offset
bool isDragging_ = false;                    // Dragging state
```

#### Drag Lifecycle

1. **Start Drag (WM_LBUTTONDOWN)**
   - Save original position
   - Calculate mouse offset
   - Set drag state
   - Capture mouse
   - Call `OnDragStart()`
   - **Return true to prevent camera interference**

2. **During Drag (WM_MOUSEMOVE)**
   - Update component position
   - Check drop targets (DragDrop mode)
   - Call `OnDragEnter/Leave()` for targets
   - Maintain visual feedback

3. **End Drag (WM_LBUTTONUP)**
   - Handle based on drag mode:
     - **Move**: Keep new position
     - **MoveRevert**: Return to original
     - **DragDrop**: Process drop or revert
   - Clean up state
   - Release mouse capture

### Drop Target Implementation

Components can act as drop targets by overriding:
```cpp
class DropTargetImage : public UIImageNew {
public:
    bool CanReceiveDrop() const override { return true; }
    
    void OnDragEnter(UIComponentNew* dragged) override {
        // Show highlight effect
        this->color = D3DCOLOR_ARGB(255, 255, 255, 128);
    }
    
    void OnDragLeave(UIComponentNew* dragged) override {
        // Restore normal color
        this->color = D3DCOLOR_ARGB(255, 255, 255, 255);
    }
    
    bool OnDrop(UIComponentNew* dragged) override {
        // Process drop logic
        if (ValidateDropItem(dragged)) {
            ProcessDroppedItem(dragged);
            return true;  // Accept drop
        }
        return false;  // Reject drop
    }
};
```

## Event Handling

### Event Types
- **Click**: Button clicks
- **ValueChanged**: Slider/input changes
- **DragStart/End**: Drag operations
- **MouseEnter/Leave**: Hover states

### Event Flow
1. Windows message received
2. UISystem processes input
3. Component-specific handlers called
4. Global event handlers notified
5. Event propagation can be stopped by returning true

### Input Processing Order
```cpp
// In main loop
if (uiSystem_->HandleInput(msg)) {
    continue; // UI handled input, skip scene processing
}
sceneManager_->HandleInput(msg);
```

## Layer System and Rendering

### Layer Management
- **Scene-specific layers**: Automatically cleaned up on scene transitions
- **Persistent layers**: Remain across scene changes (HUD, debug info)
- **Priority-based rendering**: Lower priority renders first (background to foreground)

### Rendering Pipeline
```cpp
// In main render loop
d3dContext_->Clear(...);
d3dContext_->BeginScene();

sceneManager_->Render();    // Render scene first
uiSystem_->Render();        // Render UI on top

d3dContext_->EndScene();
d3dContext_->Present();
```

## Usage Examples

### Basic UI Creation

```cpp
// In scene initialization
bool GameScene::InitializeUI() {
    // Create persistent HUD layer
    IUILayer* hudLayer = uiSystem_->CreatePersistentLayer("GameHUD", 400);
    
    // Create scene-specific game UI
    IUILayer* gameUI = uiSystem_->CreateLayer("GameInterface", UILayerType::Interface);
    
    // Add draggable background image
    auto* bgImage = gameUI->CreateImage("bg.bmp", 50, 50, 200, 150, 
                                       DragMode::Move, nullptr, true);
    
    // Add interactive button
    auto pauseBtn = gameUI->CreateButton("Pause", 10, 10, 80, 30,
        [this](const UIEvent& event) {
            OnPauseButtonClicked();
        });
    
    // Add health bar to HUD
    if (hudLayer) {
        healthBar_ = hudLayer->CreateSlider(20, 20, 200, 20, 0.0f, 100.0f, 100.0f);
        hudLayer->CreateText("HP", 5, 25, 30, 20, 0xFFFF0000);
    }
    
    return true;
}
```

### Dynamic Text Updates

```cpp
// Store text component ID
std::string scoreTextId = gameUI->CreateText("Score: 0", 200, 20, 100, 20);

// Update text later
void UpdateScore(int newScore) {
    std::string scoreStr = "Score: " + std::to_string(newScore);
    uiSystem_->GetLayer("GameInterface")->UpdateText(scoreTextId, scoreStr);
}
```

### Inventory System with Drag-Drop

```cpp
class InventoryScene : public Scene {
protected:
    bool OnInitialize() override {
        SetTransparent(true); // Show game behind
        
        inventoryLayer_ = uiSystem_->CreateLayer("Inventory", UILayerType::Overlay);
        
        // Create inventory slots
        for (int i = 0; i < 40; ++i) {
            int x = 120 + (i % 8) * 70;
            int y = 140 + (i / 8) * 70;
            
            // Create drop target slot
            auto* slot = new InventorySlot();
            slot->SetPosition(x, y);
            slot->SetSize(60, 60);
            inventoryLayer_->AddComponent(slot);
            
            inventorySlots_.push_back(slot);
        }
        
        // Create draggable items
        CreateInventoryItems();
        
        return true;
    }
    
    void CreateInventoryItems() {
        // Create draggable item icons
        auto* sword = inventoryLayer_->CreateImage("sword.png", 
            130, 150, 40, 40, DragMode::DragDrop);
        auto* potion = inventoryLayer_->CreateImage("potion.png", 
            200, 150, 40, 40, DragMode::DragDrop);
    }
};
```

## Troubleshooting

### Common Issues and Solutions

#### 1. Camera Interference During Drag
**Problem**: 3D camera responds to mouse movement while dragging UI elements.

**Solution**: Ensure UIManager returns true when starting drag:
```cpp
if (component->IsDraggable()) {
    // ... start drag ...
    return true;  // IMPORTANT: Stop message propagation
}
```

#### 2. UI Components Not Draggable
**Problem**: Components don't respond to drag attempts.

**Checklist**:
- Verify `dragMode` is not `DragMode::None`
- Check component `visible` flag
- Ensure UIManager is registered first in InputHandler
- Delete old `ui_layout.json` if using outdated format

#### 3. Incorrect Position After Drag
**Problem**: Component jumps or has wrong position.

**Debug steps**:
```cpp
// Add coordinate debugging
char debugMsg[256];
sprintf_s(debugMsg, "Drag: mouse(%d,%d) -> component(%d,%d) offset(%d,%d)\n",
          mouseX, mouseY, component->relativeX, component->relativeY, 
          dragOffset_.x, dragOffset_.y);
OutputDebugStringA(debugMsg);
```

#### 4. Transparency Detection Issues
**Problem**: Can drag from transparent areas of images.

**Solution**: Enable transparency checking:
```cpp
auto* image = uiManager->CreateImage(L"icon.png", x, y, w, h, 
                                    DragMode::Move, nullptr, 
                                    false); // allowDragFromTransparent = false
```

### Debug Mode

Enable UI system debugging:
```cpp
// Show layer information
uiSystem_->PrintLayerInfo();

// Show specific layer details
auto* gameLayer = uiSystem_->GetLayer("GameUI");
if (gameLayer) {
    gameLayer->PrintComponentInfo();
}
```

Output example:
```
=== UISystem Debug Info ===
Initialized: Yes
Total Layers: 5
Total Components: 23
Focused: GameUI:pause_button

Layer Stack (render order):
  [0] Background (priority: 0) - 1 components
  [1] GameUI (priority: 200) - 8 components
  [2] HUD (priority: 400) [PERSISTENT] - 5 components
  [3] Debug (priority: 450) [PERSISTENT] - 3 components
===========================
```

## Best Practices

### 1. Layer Organization
- Use appropriate layer types for different UI purposes
- Keep HUD and debug info in persistent layers
- Use overlay layers for menus and dialogs
- Maintain reasonable component counts per layer

### 2. Event Handling
- Return true from handlers to stop propagation
- Use lambda captures carefully to avoid dangling references
- Prefer component-specific handlers over global ones
- Clean up event handlers when destroying components

### 3. Drag System Usage
- Choose appropriate drag modes for each use case
- Implement proper drop validation logic
- Provide visual feedback during drag operations
- Handle edge cases (drag outside window, etc.)

### 4. Performance Considerations
- Minimize recursive coordinate calculations
- Cache frequently accessed components
- Avoid excessive transparency checks
- Batch UI updates when possible

### 5. Scene Integration
- Initialize UI in scene's `OnInitialize()`
- Clean up scene-specific UI in `OnExit()`
- Use ServiceLocator for UI system access
- Coordinate UI state with game logic

## Integration with ServiceLocator

```cpp
// Update ServiceLocator
class ServiceLocator : public IServiceLocator {
public:
    void SetUISystem(IUISystem* uiSystem) { uiSystem_ = uiSystem; }
    IUISystem* GetUISystem() const { return uiSystem_; }
    
private:
    IUISystem* uiSystem_;
};

// Access in scenes
class GameScene : public Scene {
protected:
    bool OnInitialize() override {
        uiSystem_ = services_->GetUISystem();
        CreateGameUI();
        return true;
    }
    
private:
    IUISystem* uiSystem_;
};
```

## Advanced Features

### Parent-Child Relationships
- Children move with parents automatically
- Relative positioning within parent bounds
- Recursive visibility and interaction states
- Proper cleanup of child components

### Serialization Support
UI layouts can be saved and loaded:
```cpp
// Save current layout
uiSystem_->SaveLayout("ui_layout.json");

// Load saved layout
uiSystem_->LoadLayout("ui_layout.json");
```

### Custom Components
Extend `UIComponentNew` for specialized behavior:
```cpp
class HealthBar : public UIComponentNew {
public:
    void SetHealth(float current, float max) {
        healthPercent_ = current / max;
        UpdateVisual();
    }
    
    void Render(IDirect3DDevice9* device) override {
        // Custom rendering logic
        DrawBackground();
        DrawHealthFill(healthPercent_);
        DrawBorder();
    }
    
private:
    float healthPercent_ = 1.0f;
};
```

## Future Improvements

### Planned Enhancements
1. **Drag Preview System**: Show ghost image while dragging
2. **Drag Constraints**: Limit draggable area/snapping
3. **Animation Support**: Smooth transitions and effects
4. **Multi-Selection**: Drag multiple components together
5. **Undo/Redo**: Drag operation history
6. **Theme System**: Consistent visual styling
7. **Layout Managers**: Automatic component arrangement
8. **Accessibility**: Keyboard navigation support

## Summary

The UI system provides a robust foundation for creating interactive user interfaces with:
- **Clear separation of concerns** through layered architecture
- **Flexible drag-and-drop** with multiple modes
- **Type-safe event handling** for reliable interactions
- **Cross-scene persistence** for consistent UI elements
- **Comprehensive debugging** tools and documentation

This architecture successfully addresses the original issues of hardcoded UI creation in EngineContext, providing a modular, scene-based approach to UI management that scales with application complexity.