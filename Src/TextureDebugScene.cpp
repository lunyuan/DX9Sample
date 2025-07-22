#include "Scene.h"
#include "ServiceLocator.h"
#include "IAssetManager.h"
#include "IInputHandler.h"
#include "ID3DContext.h"
#include <d3dx9.h>
#include <DirectXMath.h>
#include <iostream>

using namespace DirectX;

// Simple test scene to debug texture loading
class TextureDebugScene : public Scene {
public:
    explicit TextureDebugScene(ServiceLocator* services)
        : Scene(services) {}

    bool OnInitialize() override {
        OutputDebugStringA("=== TextureDebugScene: Starting texture debug ===\n");
        
        auto* device = services_->GetDevice();
        auto* assetManager = services_->GetAssetManager();
        
        if (!device || !assetManager) {
            OutputDebugStringA("TextureDebugScene: Missing device or asset manager\n");
            return false;
        }

        // Test 1: Direct texture loading
        OutputDebugStringA("\n--- Test 1: Direct D3DX texture loading ---\n");
        IDirect3DTexture9* testTexture = nullptr;
        HRESULT hr = D3DXCreateTextureFromFileA(device, "Horse4.bmp", &testTexture);
        
        char msg[256];
        sprintf_s(msg, "Direct load Horse4.bmp: HRESULT=0x%08X, ptr=%p\n", hr, testTexture);
        OutputDebugStringA(msg);
        
        if (SUCCEEDED(hr) && testTexture) {
            D3DSURFACE_DESC desc;
            testTexture->GetLevelDesc(0, &desc);
            sprintf_s(msg, "Texture info: %dx%d, Format=%d\n", desc.Width, desc.Height, desc.Format);
            OutputDebugStringA(msg);
            testTexture->Release();
        }

        // Test 2: Check working directory
        OutputDebugStringA("\n--- Test 2: Working directory ---\n");
        char cwd[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, cwd);
        sprintf_s(msg, "Current directory: %s\n", cwd);
        OutputDebugStringA(msg);

        // Test 3: Load model and check textures
        OutputDebugStringA("\n--- Test 3: Load test1.x and check materials ---\n");
        auto models = assetManager->LoadAllModels("test1.x");
        sprintf_s(msg, "Loaded %zu models from test1.x\n", models.size());
        OutputDebugStringA(msg);

        for (size_t i = 0; i < models.size(); ++i) {
            if (models[i]) {
                sprintf_s(msg, "\nModel %zu:\n", i);
                OutputDebugStringA(msg);
                
                // Check materials
                auto& materials = models[i]->mesh.materials;
                sprintf_s(msg, "  Materials: %zu\n", materials.size());
                OutputDebugStringA(msg);
                
                for (size_t j = 0; j < materials.size(); ++j) {
                    sprintf_s(msg, "  Material %zu: texture=%p\n", j, materials[j].tex);
                    OutputDebugStringA(msg);
                    
                    // Check material properties
                    auto& mat = materials[j].mat;
                    sprintf_s(msg, "    Diffuse: (%.2f, %.2f, %.2f, %.2f)\n", 
                        mat.Diffuse.r, mat.Diffuse.g, mat.Diffuse.b, mat.Diffuse.a);
                    OutputDebugStringA(msg);
                }
                
                // Check vertices for UV
                if (!models[i]->mesh.vertices.empty()) {
                    OutputDebugStringA("  First 3 vertices UV:\n");
                    for (size_t v = 0; v < std::min(size_t(3), models[i]->mesh.vertices.size()); ++v) {
                        auto& vertex = models[i]->mesh.vertices[v];
                        sprintf_s(msg, "    V%zu: UV=(%.3f, %.3f)\n", v, vertex.uv.x, vertex.uv.y);
                        OutputDebugStringA(msg);
                    }
                }
            }
        }

        // Test 4: Manually set texture and test render states
        OutputDebugStringA("\n--- Test 4: Manual texture test ---\n");
        if (!models.empty() && models[0]) {
            // Force load and set texture
            IDirect3DTexture9* horseTexture = nullptr;
            hr = D3DXCreateTextureFromFileA(device, "Horse4.bmp", &horseTexture);
            if (SUCCEEDED(hr) && horseTexture) {
                OutputDebugStringA("Successfully loaded Horse4.bmp for manual test\n");
                
                // Apply to first model
                models[0]->mesh.texture = horseTexture;
                if (!models[0]->mesh.materials.empty()) {
                    models[0]->mesh.materials[0].tex = horseTexture;
                }
                
                // Store for rendering
                testModel_ = models[0];
                manualTexture_ = horseTexture;
            }
        }

        OutputDebugStringA("\n=== TextureDebugScene: Debug complete ===\n");
        return true;
    }

    void OnUpdate(float dt) override {
        // Simple rotation
        rotation_ += dt;
    }

    void OnRender() override {
        auto* device = services_->GetDevice();
        if (!device || !testModel_) return;

        // Clear
        device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                     D3DCOLOR_XRGB(64, 64, 128), 1.0f, 0);
        device->BeginScene();

        // Set up camera
        D3DXMATRIX view, proj;
        D3DXVECTOR3 eye(0, 30, -50);
        D3DXVECTOR3 at(0, 0, 0);
        D3DXVECTOR3 up(0, 1, 0);
        D3DXMatrixLookAtLH(&view, &eye, &at, &up);
        device->SetTransform(D3DTS_VIEW, &view);

        float aspect = 1280.0f / 720.0f;
        D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(45), aspect, 0.1f, 1000.0f);
        device->SetTransform(D3DTS_PROJECTION, &proj);

        // Set up world transform with rotation
        D3DXMATRIX world;
        D3DXMatrixRotationY(&world, rotation_);
        device->SetTransform(D3DTS_WORLD, &world);

        // Set up lighting
        device->SetRenderState(D3DRS_LIGHTING, TRUE);
        device->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(128, 128, 128));

        // IMPORTANT: Enable texture stages
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

        // Set sampler states
        device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

        // Force set our manual texture
        if (manualTexture_) {
            device->SetTexture(0, manualTexture_);
            
            static int frameCount = 0;
            if (frameCount++ % 60 == 0) {
                OutputDebugStringA("TextureDebugScene: Rendering with manual texture\n");
            }
        }

        // Render the model
        testModel_->mesh.Draw(device);

        device->EndScene();
        device->Present(nullptr, nullptr, nullptr, nullptr);
    }

    void OnCleanup() override {
        if (manualTexture_) {
            manualTexture_->Release();
            manualTexture_ = nullptr;
        }
    }

private:
    std::shared_ptr<ModelData> testModel_;
    IDirect3DTexture9* manualTexture_ = nullptr;
    float rotation_ = 0.0f;
};