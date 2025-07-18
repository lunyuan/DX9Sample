#include "AnimationPlayer.h"
using namespace DX;

void AnimationPlayer::ComputeGlobalTransforms(
  const Skeleton& skel,
  const SkeletonAnimation& anim,
  float time,
  std::vector<XMFLOAT4X4>& globals) {
  size_t n = skel.joints.size();
  globals.assign(n, XMFLOAT4X4());
  for (size_t i = 0; i < n; ++i) {
    const auto& channel = anim.channels[i];
    if (channel.empty()) continue;
    size_t prev = 0, next = 0;
    for (size_t k = 0; k < channel.size(); ++k) {
      if (channel[k].time <= time) prev = k;
      if (channel[k].time >= time) { next = k; break; }
    }
    const auto& kf0 = channel[prev];
    const auto& kf1 = channel[next];
    float t0 = kf0.time, t1 = kf1.time;
    float factor = (t1 - t0 > 0) ? (time - t0) / (t1 - t0) : 0.0f;
    // Decompose transform matrices
    DirectX::XMVECTOR s0, r0v, t0v;
    DirectX::XMVECTOR s1, r1v, t1v;
    DirectX::XMMatrixDecompose(&s0, &r0v, &t0v, DirectX::XMLoadFloat4x4(&kf0.transform));
    DirectX::XMMatrixDecompose(&s1, &r1v, &t1v, DirectX::XMLoadFloat4x4(&kf1.transform));
    // Interpolate
    DirectX::XMVECTOR s = DirectX::XMVectorLerp(s0, s1, factor);
    DirectX::XMVECTOR r = DirectX::XMQuaternionSlerp(r0v, r1v, factor);
    DirectX::XMVECTOR t = DirectX::XMVectorLerp(t0v, t1v, factor);
    // Reconstruct local matrix
    DirectX::XMMATRIX local = DirectX::XMMatrixScalingFromVector(s)
      * DirectX::XMMatrixRotationQuaternion(r)
      * DirectX::XMMatrixTranslationFromVector(t);
    DirectX::XMStoreFloat4x4(&globals[i], local);
  }
  // Combine with parent transforms
  for (size_t i = 0; i < n; ++i) {
    int p = skel.joints[i].parentIndex;
    if (p >= 0) {
      DirectX::XMMATRIX parent = DirectX::XMLoadFloat4x4(&globals[p]);
      DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&globals[i]);
      DirectX::XMStoreFloat4x4(&globals[i], DirectX::XMMatrixMultiply(local, parent));
    }
  }
}
