#pragma once
#include "Skeleton.h"
#include <vector>
#include <DirectXMath.h>
namespace DX = DirectX;

class AnimationPlayer {
public:
  // 使用 SkeletonAnimation
  static void ComputeGlobalTransforms(
    const Skeleton& skel,
    const SkeletonAnimation& anim,
    float time,
    std::vector<DX::XMFLOAT4X4>& globals
  );
};