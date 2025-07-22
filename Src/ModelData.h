#pragma once
#include <memory>
#include <string>
#include <map>
#include "SkinMesh.h" 
#include "Skeleton.h"

struct ModelData {
  SkinMesh mesh;
  Skeleton skeleton;
  std::shared_ptr<ID3DXAnimationController> animController;
  
  // Texture loading control
  bool useOriginalTextures = false;  // Default: don't use textures from model file
};
