#include "WorldFormat/CMetroidModelInstance.hpp"

#include "Kyoto/Basics/CBasics.hpp"
#include "Kyoto/Streams/CInputStream.hpp"
#include "MetaRender/IRenderer.hpp"

#ifdef TARGET_PC
// Disc data is big-endian.
static CTransform4f TransformFromData(const void* data) {
  float values[12];
  const float* source = static_cast< const float* >(data);
  for (int i = 0; i < 12; ++i) {
    values[i] = CBasics::SwapBytes(source[i]);
  }
  return CTransform4f(values[0], values[1], values[2], values[3], values[4], values[5], values[6],
                      values[7], values[8], values[9], values[10], values[11]);
}
#else
static const CTransform4f& TransformFromData(const void* data) {
  return *static_cast< const CTransform4f* >(data);
}
#endif

static CAABox BoundingBoxFromData(const void* data) {
  float values[6];
  const float* source = static_cast< const float* >(data);
  for (int i = 0; i < 6; ++i) {
    values[i] = CBasics::SwapBytes(source[i]);
  }
  return *reinterpret_cast< const CAABox* >(values);
}

CMetroidModelInstance::CMetroidModelInstance(
    const void* header, const void* materials, const void* positions, const void* normals,
    const void* colors, const void* texCoords, const void* packedTexCoords,
#ifdef TARGET_PC
    const rstl::vector< void* >& surfaces, const void* const& section1, const void* const& section2,
    uint positionsSize, uint normalsSize, uint colorsSize, uint texCoordsSize,
    uint packedTexCoordsSize)
: mVisorFlags(CBasics::SwapBytes(*static_cast< const uint* >(header)))
#else
    const rstl::vector< void* >& surfaces, const void* const& section1, const void* const& section2)
: mVisorFlags(*static_cast< const uint* >(header))
#endif
, mWorldTransform(TransformFromData(static_cast< const uchar* >(header) + sizeof(uint)))
, mWorldBounds(BoundingBoxFromData(static_cast< const uchar* >(header) + sizeof(CTransform4f) +
                                   sizeof(uint)))
, mMaterialData(materials)
, mSurfaces(surfaces)
, mPositions(positions)
, mNormals(normals)
, mColors(colors)
, mTexCoords(texCoords)
, mPackedTexCoords(packedTexCoords)
, x74_(section1)
#ifdef TARGET_PC
, mSurfaceGroups(section2)
, mPositionsSize(positionsSize)
, mNormalsSize(normalsSize)
, mColorsSize(colorsSize)
, mTexCoordsSize(texCoordsSize)
, mPackedTexCoordsSize(packedTexCoordsSize) {}
#else
, mSurfaceGroups(section2) {}
#endif

SAreaSurface::SAreaSurface(CInputStream& in)
: mBounds(in)
, mModelIndex(in.ReadInt16())
, mSurfaceGroupIndex(in.ReadInt16())
, x1c_(in.ReadInt16())
, x1e_(in.ReadInt16()) {}

ushort CMetroidModelInstance::CSurfaceGroups::GetSurfaceCount(int group) const {
  const ushort count = mData[group + 1];
  if (group == 0) {
    return count;
  }
  return count - mData[group];
}

const ushort* CMetroidModelInstance::CSurfaceGroups::GetSurfaceIndices(int group) const {
  const int start = group != 0 ? mData[group] : 0;
  return mData + (mData[0] + 1) + start;
}
