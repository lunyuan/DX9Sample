#pragma once
#include <vector>
#include <string>
#include <DirectXMath.h>

struct SkeletonJoint {
  std::string name;
  int parentIndex;
  DirectX::XMFLOAT4X4 bindPoseInverse;
};

struct SkeletonAnimationKey {
  float time;
  DirectX::XMFLOAT4X4 transform;
};

struct SkeletonAnimation {
  std::string name;
  float duration;
  std::vector<std::vector<SkeletonAnimationKey>> channels;
};

class Skeleton {
public:
  std::vector<SkeletonJoint> joints;
  std::vector<SkeletonAnimation> animations;
};