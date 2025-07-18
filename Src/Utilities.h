#pragma once
#include "XFileTypes.h"
#include <vector>
#include <DirectXMath.h>

// Step 1: 更新 CombinedTransform
inline void UpdateCombined(FrameEx* frame, const DirectX::XMMATRIX& parent) noexcept {
  if (!frame) return;
  DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&frame->dxTransformationMatrix);
  DirectX::XMMATRIX combined = DirectX::XMMatrixMultiply(local, parent);
  DirectX::XMStoreFloat4x4(&frame->dxCombinedTransform, combined);
  if (frame->pFrameFirstChild) UpdateCombined(reinterpret_cast<FrameEx*>(frame->pFrameFirstChild), combined);
  if (frame->pFrameSibling)  UpdateCombined(reinterpret_cast<FrameEx*>(frame->pFrameSibling), parent);
}

// Step 2: 收集所有 Frame 並建立父子索引
inline void CollectFrames(FrameEx* frame, std::vector<FrameEx*>& out, std::vector<int>& parents, int parentIndex) noexcept {
  if (!frame) return;
  int idx = static_cast<int>(out.size());
  out.push_back(frame);
  parents.push_back(parentIndex);
  if (frame->pFrameFirstChild) CollectFrames(reinterpret_cast<FrameEx*>(frame->pFrameFirstChild), out, parents, idx);
  if (frame->pFrameSibling)   CollectFrames(reinterpret_cast<FrameEx*>(frame->pFrameSibling), out, parents, parentIndex);
}
