#ifndef _CCUBESURFACE
#define _CCUBESURFACE

#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"

class CCubeModel;
class CCubeSurface {
  friend class CCubeModel;

#ifdef TARGET_PC
  // Port: the disc header holds 32-bit pointers; the native one holds 64-bit pointers in a
  // packed layout, rewritten in place by ConvertSurfaceHeader.
#pragma pack(push, 1)
#endif
  struct SSurfaceData {
    CVector3f mCenter;
    uint mMaterialIndex;
    uint mDisplayListSizeAndNormalHint;
    CCubeModel* mParent;
    const void* mNextSurface;
    uint mExtraSize;
    CUnitVector3f mNormal;
    short mMatrixBank;
    short x2e_;
    CAABox mBounds;
  };
#ifdef TARGET_PC
#pragma pack(pop)
#endif

  static const CVector3f skDefaultNormal;
  const SSurfaceData* mData;

public:
#ifdef TARGET_PC
  // Converts a big-endian, 32-bit-pointer disc surface header to the native layout.
  static void ConvertSurfaceHeader(void* data);
#endif
  explicit CCubeSurface(const void* data) : mData(static_cast< const SSurfaceData* >(data)) {}

  CAABox GetBounds() const;
  const CVector3f& GetCenter() const { return mData->mCenter; }
  const CUnitVector3f& GetNormalHint() const { return mData->mNormal; }
  uint GetMaterialIndex() const { return mData->mMaterialIndex; }
  CCubeModel* GetParent() const { return mData->mParent; }
  short GetMatrixBank() const { return mData->mMatrixBank; } // Guessed name.
  short GetShadowBank() const { return mData->x2e_; }        // Target-derived grouping key.
  uint GetDisplayListSize() const { return mData->mDisplayListSizeAndNormalHint & 0x7fffffff; }
#ifdef TARGET_PC
  // The on-disc header is 0x48 bytes, not the native struct's size.
  uint GetSurfaceHeaderSize() const { return (0x4f + mData->mExtraSize) & ~31; }
#else
  uint GetSurfaceHeaderSize() const { return (sizeof(SSurfaceData) + 7 + mData->mExtraSize) & ~31; }
#endif
  const void* GetDisplayList() const {
    return reinterpret_cast< const uchar* >(mData) + GetSurfaceHeaderSize();
  }
  CCubeSurface GetNextSurface() const { return CCubeSurface(mData->mNextSurface); }
  bool IsValid() const { return mData != nullptr; }
};
#ifndef TARGET_PC
CHECK_SIZEOF(CCubeSurface, 4)
#endif

#endif // _CCUBESURFACE
