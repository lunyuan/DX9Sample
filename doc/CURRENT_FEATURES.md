# Current Features Documentation

## 1. 3D Rendering Features

### Model Loading
- **Multiple Format Support**
  - DirectX .x files with materials and textures
  - FBX files via Autodesk FBX SDK
  - glTF 2.0 files (experimental)
  
### Skeletal Animation
- **Bone-based animation system**
  - Vertex skinning with up to 4 bones per vertex
  - Animation blending support
  - HLSL shader-based skinning
  - Animation playback control

### Material System
- **Per-mesh materials**
  - Diffuse, ambient, specular properties
  - Texture mapping support
  - Multi-material meshes

### Lighting
- **Directional lights**
- **Point lights** (planned)
- **Ambient lighting**
- **Per-pixel lighting** via shaders

## 2. UI System Features

### Modern Component System

#### UIImageNew
- **Features**:
  - Image rendering with transparency
  - Draggable support
  - Can receive drag-drop
  - Parent-child relationships
  - Pixel-perfect hit testing
  - Color tinting

#### UIButtonNew
- **Features**:
  - Four-state system (Normal, Hover, Pressed, Disabled)
  - Custom images for each state
  - Text rendering
  - Click callbacks
  - Parent-child support

#### UIEditNew
- **Features**:
  - Text input with Unicode support
  - Cursor navigation
  - Background image support
  - Focus management

### Drag-Drop System
- **Complete Implementation**:
  - Left-click to start dragging
  - Visual feedback during drag (position following)
  - Drop target highlighting (yellow tint)
  - Accept/reject drop logic
  - Auto-return to original position if rejected
  - Component deletion on successful drop

### UI Serialization
- **JSON-based save/load**:
  - Preserves component hierarchy
  - Saves all properties
  - Restores event handlers
  - Maintains parent-child relationships

## 3. Scene Management

### Scene Stack System
- **Push/Pop Operations**: Navigate between scenes
- **Transparent Overlays**: Pause menus over gameplay
- **Scene Lifecycle**: Proper initialization and cleanup

### Available Scenes

#### GameScene
- **3D model rendering**
- **Camera controls**
- **UI overlay**
- **Asset loading**
- **Score and level tracking**

#### PauseScene
- **Transparent overlay**
- **Resume/Settings/Quit options**
- **Preserves game state**

#### SettingsScene
- **Configuration options**
- **Apply/Cancel functionality**

## 4. Asset Management

### Unified Asset Loading
- **Path resolution system**
- **Type detection**
- **Reference counting**
- **Memory management**
- **Thread-safe caching**

### Supported Asset Types
- **Models**: .x, .fbx, .gltf
- **Textures**: .bmp, .jpg, .png, .dds, .tga
- **Sounds**: .wav, .mp3, .ogg
- **Configs**: .json, .xml

### Hot Reload
- **Development feature**
- **Automatic file watching**
- **Cache invalidation**

## 5. Event System

### Type-Safe Events
- **Compile-time checking**
- **No string-based events**
- **Automatic cleanup**

### Event Types
- **UI Events**: Button clicks, component interactions
- **Scene Events**: Transitions, state changes
- **Asset Events**: Load completion, errors
- **Game Events**: Score changes, level ups

## 6. Configuration System

### JSON Configuration
- **Runtime modifiable**
- **Type-safe access**
- **Default values**
- **Nested structures**

### Configuration Categories
- **Graphics settings**
- **Game parameters**
- **UI configuration**
- **Debug options**

## 7. Input Handling

### Mouse Input
- **Full mouse support**
- **UI interaction priority**
- **Camera control when not on UI**
- **Drag-drop operations**

### Keyboard Input
- **Text input for edit boxes**
- **Hotkeys for menus**
- **Camera controls**

## 8. Camera System

### Camera Controller
- **Mouse-based rotation**
- **Keyboard movement**
- **Smooth transitions**
- **Configurable sensitivity**

## 9. Texture Management

### Features
- **Thread-safe caching**
- **Automatic format detection**
- **Mipmap generation**
- **Reference counting**
- **Memory optimization**

### Transparency Support
- **Alpha channel for PNG**
- **Color key for BMP (green = transparent)**
- **Pixel-perfect hit testing**

## 10. Debug Features

### Logging System
- **Filtered debug output**
- **Performance metrics**
- **Asset loading traces**

### Development Tools
- **Hot reload**
- **Scene inspection**
- **Memory usage tracking**

## 11. Special Features

### Parent-Child UI System
- **Hierarchical components**
- **Grouped movement**
- **Relative positioning**
- **Recursive rendering**

### Layer System
- **Multiple rendering layers**
- **Priority-based sorting**
- **Per-layer visibility**
- **Alpha blending per layer**

### Smart Pointer Architecture
- **No manual memory management**
- **Automatic cleanup**
- **Cycle prevention**
- **Thread safety**

## 12. Current UI Layout Example

### GameScene UI Structure
```
Root
├── bg.png (Draggable container, receives drops)
│   ├── PAUSE button (bt.bmp background)
│   │   └── 7.png (Draggable item)
│   └── Test text
├── b-kuang.png (Draggable container, receives drops)
│   └── TEST button
└── Standalone button
```

### Drag-Drop Behavior
1. **7.png** can be dragged from the PAUSE button
2. Can be dropped onto **bg.png** or **b-kuang.png**
3. If dropped on valid target: item is deleted
4. If dropped elsewhere: returns to original position
5. Visual feedback during hover over drop targets