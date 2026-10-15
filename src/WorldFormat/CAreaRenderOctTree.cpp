#include "WorldFormat/CAreaRenderOctTree.hpp"

#include "Kyoto/Basics/CBasics.hpp"

static const int skChildCounts[] = {0, 2, 2, 4, 2, 4, 4, 8};
static const int skAxes[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, -1, -1, -1, 0, 2, 1, 2, 0, 1,
};

inline const CAreaRenderOctTree::Node* CAreaRenderOctTree::GetNode(int index) const {
  return reinterpret_cast< const Node* >(mEntries + CBasics::SwapBytes(mIndirectionTable[index]));
}

#ifdef TARGET_PC
// The AROT bounds are big-endian floats on disc.
static CAABox BoundsFromDisc(const float* f) {
  return CAABox(CBasics::SwapBytes(f[0]), CBasics::SwapBytes(f[1]), CBasics::SwapBytes(f[2]),
                CBasics::SwapBytes(f[3]), CBasics::SwapBytes(f[4]), CBasics::SwapBytes(f[5]));
}
#endif

CAreaRenderOctTree::CAreaRenderOctTree(const rstl::auto_ptr< const uchar >& buffer)
: mBuffer(buffer)
, mBitmapCount(CBasics::SwapBytes(*reinterpret_cast< const uint* >(buffer.get() + 8)))
, mMeshCount(CBasics::SwapBytes(*reinterpret_cast< const uint* >(buffer.get() + 12)))
, mNodeCount(CBasics::SwapBytes(*reinterpret_cast< const uint* >(buffer.get() + 16)))
, mBitmapWordCount((mMeshCount + 31) / 32)
#ifdef TARGET_PC
, mBounds(BoundsFromDisc(reinterpret_cast< const float* >(buffer.get() + 20)))
#else
, mBounds(*reinterpret_cast< const CAABox* >(buffer.get() + 20))
#endif
, mBitmaps(reinterpret_cast< const uint* >(buffer.get() + 64))
, mIndirectionTable(mBitmaps + mBitmapCount * mBitmapWordCount)
, mEntries(reinterpret_cast< const uchar* >(mIndirectionTable + mNodeCount)) {}

int CAreaRenderOctTree::Node::GetChildCount() const { return skChildCounts[mFlags]; }

CAABox CAreaRenderOctTree::Node::GetNodeBounds(const CAABox& bounds, int childIndex) const {
  CVector3f min = bounds.GetMinPoint();
  CVector3f max = bounds.GetMaxPoint();
  const uint flags = mFlags;
  switch (flags) {
  case kS_Leaf:
  default:
    break;
  case kS_X: {
    const float center = 0.5f * (max.GetX() + min.GetX());
    if (childIndex == 0) {
      max.SetX(center);
    } else {
      min.SetX(center);
    }
    break;
  }
  case kS_Y: {
    const float center = 0.5f * (max.GetY() + min.GetY());
    if (childIndex == 0) {
      max.SetY(center);
    } else {
      min.SetY(center);
    }
    break;
  }
  case kS_Z: {
    const float center = 0.5f * (max.GetZ() + min.GetZ());
    if (childIndex == 0) {
      max.SetZ(center);
    } else {
      min.SetZ(center);
    }
    break;
  }
  case kS_XY:
  case kS_XZ:
  case kS_ZX: {
    const CVector3f center = bounds.GetCenterPoint();
    const int idx = flags * 3;
    const int a = skAxes[idx];
    const int b = skAxes[idx + 1];
    switch (childIndex) {
    case 0:
      max[a] = center[a];
      max[b] = center[b];
      break;
    case 1:
      min[a] = center[a];
      max[b] = center[b];
      break;
    case 2:
      min[b] = center[b];
      max[a] = center[a];
      break;
    case 3:
      min[a] = center[a];
      min[b] = center[b];
      break;
    }
    break;
  }
  case kS_XYZ: {
    const CVector3f center = bounds.GetCenterPoint();
    for (int i = 0; i < 3; ++i) {
      if (childIndex & (1 << i)) {
        min[i] = center[i];
      } else {
        max[i] = center[i];
      }
    }
    break;
  }
  }

  return CAABox(min, max);
}

void CAreaRenderOctTree::FindOverlappingModels(rstl::vector< uint >& bitmap,
                                               const CAABox& bounds) const {
  bitmap.resize(mBitmapWordCount, 0);
  GetNode(0)->RecursiveBuildOverlaps(bitmap.data(), *this, mBounds, bounds);
}

void CAreaRenderOctTree::FindOverlappingModels(uint* bitmap, const CAABox& bounds) const {
  GetNode(0)->RecursiveBuildOverlaps(bitmap, *this, mBounds, bounds);
}

void CAreaRenderOctTree::Node::RecursiveBuildOverlaps(uint* bitmap, const CAreaRenderOctTree& tree,
                                                      const CAABox& bounds,
                                                      const CAABox& testBounds) const {
  if (testBounds.DoBoundsOverlap(bounds)) {
    if (mFlags == kS_Leaf || bounds.Inside(testBounds)) {
      const ushort bitmapIndex = CBasics::SwapBytes(mBitmapIndex);
      const uint* nodeBitmap = &tree.mBitmaps[bitmapIndex * tree.mBitmapWordCount];
      for (uint i = 0; i < tree.mBitmapWordCount; ++i) {
        bitmap[i] |= CBasics::SwapBytes(nodeBitmap[i]);
      }
    } else {
      const int childCount = GetChildCount();
      for (int i = 0; i < childCount; ++i) {
#ifdef TARGET_PC
        const Node* child = tree.GetNode(CBasics::SwapBytes(mChildren[i]));
#else
        const Node* child = tree.GetNode(mChildren[i]);
#endif
        child->RecursiveBuildOverlaps(bitmap, tree, GetNodeBounds(bounds, i), testBounds);
      }
    }
  }
}

bool CAreaRenderOctTree::TestBit(const uint* bitmap, int bitIndex) {
  return (bitmap[bitIndex >> 5] & (1 << (bitIndex & 31))) != 0;
}
