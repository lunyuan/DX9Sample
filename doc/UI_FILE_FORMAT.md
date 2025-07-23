# UI File Format Documentation

## Overview
The UI system uses JSON format to save and load UI layouts. The file includes all UI components with their properties, hierarchy relationships, and visual settings.

## File Structure

```json
{
    "version": "1.0",           // File format version
    "type": "UISystem",         // File type identifier
    "layers": [],               // UI layers (reserved for future use)
    "components": [...]         // Root UI components array
}
```

## Component Structure

### Base Component Properties
All UI components share these common properties:

```json
{
    "id": 0,                    // Unique component ID
    "name": "ComponentName",    // Component name for lookup
    "relativeX": 100,          // X position relative to parent
    "relativeY": 100,          // Y position relative to parent
    "width": 100,              // Component width
    "height": 50,              // Component height
    "visible": true,           // Visibility flag
    "enabled": true,           // Enable/disable flag
    "componentType": "...",    // Component type identifier
    "children": []             // Child components array
}
```

### Component Types

#### 1. UIImageNew (Image Component)
```json
{
    "componentType": "UIImageNew",
    "imagePath": "image.png",           // Image file path
    "color": 4294967295,               // Color tint (ARGB as uint32)
    "useTransparency": true,           // Use alpha channel
    "draggable": false,                // Can be dragged
    "allowDragFromTransparent": false  // Allow drag from transparent pixels
}
```

#### 2. UIButtonNew (Button Component)
```json
{
    "componentType": "UIButtonNew",
    "text": "Button Text",              // Button label
    "normalImage": "normal.png",        // Normal state image
    "hoverImage": "hover.png",          // Hover state image
    "pressedImage": "pressed.png",      // Pressed state image
    "disabledImage": "disabled.png",    // Disabled state image
    "textColor": 4278190080,           // Text color (ARGB)
    "backgroundColor": 4290822336       // Background color (ARGB)
}
```

#### 3. UIEditNew (Edit Box Component)
```json
{
    "componentType": "UIEditNew",
    "text": "Initial text",             // Current text content
    "backgroundImage": "edit_bg.png",   // Background image
    "textColor": 4278190080,           // Text color
    "backgroundColor": 4294967295,      // Background color
    "borderColor": 4286611584,         // Border color
    "maxLength": 256                   // Maximum text length
}
```

## Color Format
Colors are stored as 32-bit unsigned integers in ARGB format:
- Alpha: bits 24-31
- Red: bits 16-23
- Green: bits 8-15
- Blue: bits 0-7

Example: 0xFFFFFFFF = White with full opacity

## Hierarchy Structure
Components can have parent-child relationships:
- Root components are at the top level of the "components" array
- Child components are in the "children" array of their parent
- Positions (relativeX, relativeY) are relative to the parent

## Example: Complete UI Layout

```json
{
    "version": "1.0",
    "type": "UISystem",
    "components": [
        {
            "id": 0,
            "name": "MainPanel",
            "relativeX": 100,
            "relativeY": 100,
            "width": 400,
            "height": 300,
            "componentType": "UIImageNew",
            "imagePath": "panel_bg.png",
            "draggable": true,
            "children": [
                {
                    "id": 1,
                    "name": "CloseButton",
                    "relativeX": 350,
                    "relativeY": 10,
                    "width": 40,
                    "height": 40,
                    "componentType": "UIButtonNew",
                    "text": "X",
                    "normalImage": "btn_close.png"
                }
            ]
        }
    ]
}
```

## Loading and Saving

### To Save UI Layout:
```cpp
UISerializer::SaveToFile(uiManager, "ui_layout.json");
```

### To Load UI Layout:
```cpp
UISerializer::LoadFromFile(uiManager, "ui_layout.json");
```

## Notes
1. Component IDs must be unique within the layout
2. File paths are relative to the application's working directory
3. Empty strings for image paths mean no image is used
4. The onClick callback for buttons cannot be serialized and must be reconnected after loading