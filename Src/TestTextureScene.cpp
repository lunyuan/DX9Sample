#include "TestTextureScene.h"
#include <d3dx9.h>
#include <iostream>

TestTextureScene::TestTextureScene(ServiceLocator* services)
    : Scene(services) {}

bool TestTextureScene::OnInitialize() {
    auto* device = services_->GetDevice();
    if (!device) return false;

    // 創建一個簡單的四邊形來測試貼圖
    struct SimpleVertex {
        float x, y, z;
        float u, v;
    };

    SimpleVertex vertices[] = {
        { -10.0f,  10.0f, 0.0f, 0.0f, 0.0f },  // 左上
        {  10.0f,  10.0f, 0.0f, 1.0f, 0.0f },  // 右上
        { -10.0f, -10.0f, 0.0f, 0.0f, 1.0f },  // 左下
        {  10.0f, -10.0f, 0.0f, 1.0f, 1.0f },  // 右下
    };

    // 創建頂點緩衝區
    device->CreateVertexBuffer(
        sizeof(vertices),
        D3DUSAGE_WRITEONLY,
        0,
        D3DPOOL_DEFAULT,
        &vertexBuffer_,
        nullptr
    );

    void* data;
    vertexBuffer_->Lock(0, sizeof(vertices), &data, 0);
    memcpy(data, vertices, sizeof(vertices));
    vertexBuffer_->Unlock();

    // 載入 Horse4.bmp
    HRESULT hr = D3DXCreateTextureFromFileA(device, "Horse4.bmp", &texture_);
    
    char msg[256];
    sprintf_s(msg, "TestTextureScene: Load Horse4.bmp result: 0x%08X, texture=%p\n", hr, texture_);
    OutputDebugStringA(msg);

    return SUCCEEDED(hr);
}

void TestTextureScene::OnUpdate(float dt) {
    rotation_ += dt * 0.5f;
}

void TestTextureScene::OnRender() {
    auto* device = services_->GetDevice();
    if (!device) return;

    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                  D3DCOLOR_XRGB(64, 64, 64), 1.0f, 0);
    device->BeginScene();

    // 設置相機
    D3DXMATRIX view, proj;
    D3DXVECTOR3 eye(0, 0, -30);
    D3DXVECTOR3 at(0, 0, 0);
    D3DXVECTOR3 up(0, 1, 0);
    D3DXMatrixLookAtLH(&view, &eye, &at, &up);
    device->SetTransform(D3DTS_VIEW, &view);

    D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(45), 16.0f/9.0f, 0.1f, 1000.0f);
    device->SetTransform(D3DTS_PROJECTION, &proj);

    // 旋轉
    D3DXMATRIX world;
    D3DXMatrixRotationY(&world, rotation_);
    device->SetTransform(D3DTS_WORLD, &world);

    // 設置渲染狀態
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    // 設置貼圖
    device->SetTexture(0, texture_);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    // 設置貼圖階段狀態
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

    // 設置 FVF
    device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);

    // 繪製
    device->SetStreamSource(0, vertexBuffer_, 0, sizeof(SimpleVertex));
    device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    // 顯示調試信息
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {
        char msg[256];
        sprintf_s(msg, "TestTextureScene: Rendering with texture=%p\n", texture_);
        OutputDebugStringA(msg);
    }

    device->EndScene();
    device->Present(nullptr, nullptr, nullptr, nullptr);
}

void TestTextureScene::OnCleanup() {
    if (vertexBuffer_) {
        vertexBuffer_->Release();
        vertexBuffer_ = nullptr;
    }
    if (texture_) {
        texture_->Release();
        texture_ = nullptr;
    }
}