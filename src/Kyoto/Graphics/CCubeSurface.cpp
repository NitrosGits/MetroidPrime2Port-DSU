#include "Kyoto/Graphics/CCubeSurface.hpp"

const CVector3f CCubeSurface::skDefaultNormal(1.f, 0.f, 0.f);

CAABox CCubeSurface::GetBounds() const {
  if (mData->mExtraSize != 0) {
    return mData->mBounds;
  }

  return CAABox(mData->mCenter, mData->mCenter);
}

#ifdef TARGET_PC
#include "Kyoto/Basics/CBasics.hpp"

#include <string.h>

void CCubeSurface::ConvertSurfaceHeader(void* rawData) {
  uchar* data = static_cast< uchar* >(rawData);
  float center[3];
  float normal[3];
  for (int i = 0; i < 3; ++i) {
    center[i] = CBasics::SwapBytes(*reinterpret_cast< const float* >(data + i * 4));
    normal[i] = CBasics::SwapBytes(*reinterpret_cast< const float* >(data + 0x20 + i * 4));
  }
  const uint materialIndex = CBasics::SwapBytes(*reinterpret_cast< const uint* >(data + 0xc));
  const uint displayListSize = CBasics::SwapBytes(*reinterpret_cast< const uint* >(data + 0x10));
  const uint extraSize = CBasics::SwapBytes(*reinterpret_cast< const uint* >(data + 0x1c));
  const short matrixBank = CBasics::SwapBytes(*reinterpret_cast< const short* >(data + 0x2c));
  const short shadowBank = CBasics::SwapBytes(*reinterpret_cast< const short* >(data + 0x2e));
  float bounds[6];
  if (extraSize != 0) {
    for (int i = 0; i < 6; ++i) {
      bounds[i] = CBasics::SwapBytes(*reinterpret_cast< const float* >(data + 0x30 + i * 4));
    }
  }

  SSurfaceData* surface = static_cast< SSurfaceData* >(rawData);
  memcpy(&surface->mCenter, center, sizeof(center));
  surface->mMaterialIndex = materialIndex;
  surface->mDisplayListSizeAndNormalHint = displayListSize;
  surface->mParent = nullptr;
  surface->mNextSurface = nullptr;
  surface->mExtraSize = extraSize;
  memcpy(&surface->mNormal, normal, sizeof(normal));
  surface->mMatrixBank = matrixBank;
  surface->x2e_ = shadowBank;
  if (extraSize != 0) {
    memcpy(&surface->mBounds, bounds, sizeof(bounds));
  }
}
#endif
