#pragma once
#include "Scene.h"
#include <d3d9.h>

// 簡單的貼圖測試場景
class TestTextureScene : public Scene {
public:
    explicit TestTextureScene(ServiceLocator* services);
    
    bool OnInitialize() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnCleanup() override;

private:
    IDirect3DVertexBuffer9* vertexBuffer_ = nullptr;
    IDirect3DTexture9* texture_ = nullptr;
    float rotation_ = 0.0f;
};