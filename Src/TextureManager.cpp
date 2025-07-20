#include "TextureManager.h"

// Factory
std::unique_ptr<ITextureManager> CreateTextureManager(
  ComPtr<IDirect3DDevice9> device
) {
  if (!device.Get()) {
    throw std::invalid_argument("CreateTextureManager: device 為 nullptr");
  }
  auto mgr = std::make_unique<TextureManager>(device);
  mgr->Initialize(device);
  return mgr;
}

TextureManager::TextureManager(ComPtr<IDirect3DDevice9> device) noexcept
  : device_(device) {
}

void TextureManager::Initialize(ComPtr<IDirect3DDevice9> device) {
  if (!device.Get()) {
    throw std::invalid_argument("TextureManager::Initialize: device 為 nullptr");
  }

  std::scoped_lock lock{ mutex_ };
  device_ = device;
  cache_.clear();
}

std::shared_ptr<IDirect3DBaseTexture9> TextureManager::Load(
  const std::filesystem::path& filepath
) {
  if (!device_.Get()) {
    throw std::logic_error("TextureManager::Load: Device 未初始化");
  }
  if (filepath.empty()) {
    throw std::invalid_argument("TextureManager::Load: filepath 不能為空");
  }
  if (!std::filesystem::exists(filepath)) {
    throw std::runtime_error(std::format("TextureManager::Load: 檔案不存在 {}", filepath.string()));
  }

  const std::string key = filepath.string();

  {
    std::shared_lock lock{ mutex_ };
    auto it = cache_.find(key);
    if (it != cache_.end()) {
      return it->second;
    }
  }

  //  從檔案載入貼圖 (Managed Pool) - 對bg.bmp使用綠色色彩鍵
  IDirect3DBaseTexture9* rawTex = nullptr;
  D3DCOLOR colorKey = 0; // 預設無色彩鍵
  
  // 檢查是否為bg.bmp或bt.bmp，使用綠色色彩鍵
  std::string filename = filepath.filename().string();
  if (filename == "bg.bmp" || filename == "bt.bmp") {
    colorKey = D3DCOLOR_XRGB(0, 255, 0); // 純綠色作為透明色
  }
  
  HRESULT hr = D3DXCreateTextureFromFileExW(
    device_.Get(),
    filepath.wstring().c_str(),
    D3DX_DEFAULT, D3DX_DEFAULT,
    D3DX_DEFAULT, 0,
    D3DFMT_UNKNOWN,
    D3DPOOL_MANAGED,
    D3DX_DEFAULT, D3DX_DEFAULT,
    colorKey, nullptr, nullptr,
    reinterpret_cast<IDirect3DTexture9**>(&rawTex)
  );
  if (FAILED(hr) || rawTex == nullptr) {
    throw std::runtime_error(std::format("TextureManager::Load: 載入貼圖失敗 {} (HRESULT=0x{:08X})", key, static_cast<UINT>(hr)));
  }

  //  建立 shared_ptr 並設定釋放器
  auto deleter = [](IDirect3DBaseTexture9* p) noexcept {
    if (p) p->Release();
    };
  std::shared_ptr<IDirect3DBaseTexture9> texPtr{ rawTex, deleter };

  // 寫鎖寫入快取
  {
    std::scoped_lock lock{ mutex_ };
    cache_.emplace(key, texPtr);
  }

  return texPtr;
}

std::shared_ptr<IDirect3DBaseTexture9> TextureManager::Get(
  std::string_view key
) const {
  if (key.empty()) {
    return nullptr;
  }
  std::shared_lock lock{ mutex_ };
  auto it = cache_.find(std::string{ key });
  return it != cache_.end() ? it->second : nullptr;
}

void TextureManager::Clear() noexcept {
  std::scoped_lock lock{ mutex_ };
  cache_.clear();
}
