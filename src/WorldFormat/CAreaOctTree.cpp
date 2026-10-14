#include "WorldFormat/CAreaOctTree.hpp"
#include "Kyoto/Streams/CMemoryInStream.hpp"

#ifdef TARGET_PC
#include "Kyoto/Basics/CBasics.hpp"
#include <string.h>
#endif

static CAABox BoxFromIndex(int index, const CVector3f& min, const CVector3f& center,
                           const CVector3f& max) {
  switch (index) {
  case 0:
    return CAABox(min, center);
  case 1:
    return CAABox(CVector3f(center.GetX(), min.GetY(), min.GetZ()),
                  CVector3f(max.GetX(), center.GetY(), center.GetZ()));
  case 2:
    return CAABox(CVector3f(min.GetX(), center.GetY(), min.GetZ()),
                  CVector3f(center.GetX(), max.GetY(), center.GetZ()));
  case 3:
    return CAABox(CVector3f(center.GetX(), center.GetY(), min.GetZ()),
                  CVector3f(max.GetX(), max.GetY(), center.GetZ()));
  case 4:
    return CAABox(CVector3f(min.GetX(), min.GetY(), center.GetZ()),
                  CVector3f(center.GetX(), center.GetY(), max.GetZ()));
  case 5:
    return CAABox(CVector3f(center.GetX(), min.GetY(), center.GetZ()),
                  CVector3f(max.GetX(), center.GetY(), max.GetZ()));
  case 6:
    return CAABox(CVector3f(min.GetX(), center.GetY(), center.GetZ()),
                  CVector3f(center.GetX(), max.GetY(), max.GetZ()));
  case 7:
    return CAABox(center, max);
  default:
    return CAABox(min, max);
  }
}

CAreaOctTree::Node CAreaOctTree::Node::GetChild(int index) const {
  const ETreeType type = GetChildType(index);
  const uint* offsets = reinterpret_cast< const uint* >(mPtr + sizeof(uint));
  const void* child = mPtr + 9 * sizeof(uint) + offsets[index];
  if (type == kTT_Leaf) {
    return Node(child, *static_cast< const CAABox* >(child), mOwner, type);
  }

  const CVector3f center = 0.5f * (mAabb.GetMinPoint() + mAabb.GetMaxPoint());
  return Node(child, BoxFromIndex(index, mAabb.GetMinPoint(), center, mAabb.GetMaxPoint()), mOwner,
              type);
}

CAreaOctTree::TriListReference CAreaOctTree::Node::GetTriangleArray() const {
  static const ushort skDeadArray[2] = {0, 0};
  if (mNodeType != kTT_Leaf) {
    return TriListReference(skDeadArray);
  }
  return TriListReference(mPtr);
}

CAreaOctTree::CAreaOctTree(const CAABox& bounds, Node::ETreeType treeType, const uchar* buffer,
                           const void* treeBuffer, int materialCount, int vertexCount,
                           int edgeCount, int triangleCount, const u64* materials,
                           const uchar* vertexMaterials, const uchar* edgeMaterials,
                           const uchar* surfaceMaterials, const CCollisionEdge* edges,
                           const ushort* surfaceIndices, const ushort* extraIndices,
                           const CVector3f* vertices)
: CCollisionPrimitiveData(materialCount, vertexCount, edgeCount, triangleCount, materials,
                          vertexMaterials, edgeMaterials, surfaceMaterials, edges, surfaceIndices,
                          extraIndices, vertices, false)
, mAabb(bounds)
, mTreeType(treeType)
, mBuf(buffer)
, mTreeBuf(treeBuffer) {}

#ifdef TARGET_PC
// Port: the collision resource is big-endian and read in place. Convert it to host order once,
// before the tree and arrays are used. Arrays after the byte-sized material lists may be
// unaligned, hence the memcpy-based helpers.
static uint SwapU32At(uchar* p) {
  uint v;
  memcpy(&v, p, sizeof(v));
  v = CBasics::SwapBytes(v);
  memcpy(p, &v, sizeof(v));
  return v;
}

static void SwapU16Array(uchar* p, uint count) {
  for (uint i = 0; i < count; ++i, p += sizeof(ushort)) {
    ushort v;
    memcpy(&v, p, sizeof(v));
    v = CBasics::SwapBytes(v);
    memcpy(p, &v, sizeof(v));
  }
}

static void SwapTreeToHost(uchar* node, CAreaOctTree::Node::ETreeType type) {
  if (type == CAreaOctTree::Node::kTT_Leaf) {
    for (int i = 0; i < 6; ++i) {
      SwapU32At(node + i * sizeof(float));
    }
    SwapU16Array(node + sizeof(CAABox), 1);
    ushort count;
    memcpy(&count, node + sizeof(CAABox), sizeof(count));
    SwapU16Array(node + sizeof(CAABox) + sizeof(ushort), count);
  } else if (type == CAreaOctTree::Node::kTT_Branch) {
    SwapU16Array(node, 1);
    ushort flags;
    memcpy(&flags, node, sizeof(flags));
    for (int i = 0; i < 8; ++i) {
      const uint offset = SwapU32At(node + sizeof(uint) * (i + 1));
      SwapTreeToHost(node + 9 * sizeof(uint) + offset,
                     static_cast< CAreaOctTree::Node::ETreeType >((flags >> (2 * i)) & 3));
    }
  }
}

static void SwapCollisionArraysToHost(uchar* p) {
  const uint materialCount = SwapU32At(p);
  p += sizeof(uint);
  for (uint i = 0; i < materialCount; ++i, p += sizeof(u64)) {
    u64 v;
    memcpy(&v, p, sizeof(v));
    v = __builtin_bswap64(v);
    memcpy(p, &v, sizeof(v));
  }
  for (int i = 0; i < 3; ++i) {
    p += sizeof(uint) + SwapU32At(p);
  }
  const uint edgeCount = SwapU32At(p);
  p += sizeof(uint);
  SwapU16Array(p, edgeCount * 2);
  p += edgeCount * sizeof(CCollisionEdge);
  const uint indexCount = SwapU32At(p) / 3 * 3;
  p += sizeof(uint);
  SwapU16Array(p, indexCount);
  p += indexCount * sizeof(ushort);
  SwapU32At(p);
  p += sizeof(uint);
  SwapU16Array(p, indexCount);
  p += indexCount * sizeof(ushort);
  const uint vertexCount = SwapU32At(p);
  p += sizeof(uint);
  for (uint i = 0; i < vertexCount * 3; ++i, p += sizeof(float)) {
    SwapU32At(p);
  }
}
#endif

void CAreaOctTree::MakeFromMemory(void* buffer, uint bufferLength, CAreaOctTree** treeOut,
                                  bool* valid) {
  CMemoryInStream in(buffer, bufferLength, CMemoryInStream::kOS_NotOwned);
  in.ReadInt32();
  in.ReadInt32();
  CAABox bounds(in);
  Node::ETreeType treeType = static_cast< Node::ETreeType >(in.ReadInt32());
  uint treeSize = in.ReadInt32();
  uchar* treeBuffer = static_cast< uchar* >(buffer) + in.GetReadPosition();
#ifdef TARGET_PC
  SwapTreeToHost(treeBuffer, treeType);
  SwapCollisionArraysToHost(treeBuffer + treeSize);
#endif

  uint* materialHeader = reinterpret_cast< uint* >(treeBuffer + treeSize);
  uint materialCount = *materialHeader;
  u64* materials = reinterpret_cast< u64* >(materialHeader + 1);
  uint* vertexMaterialHeader = reinterpret_cast< uint* >(materials + materialCount);
  uchar* vertexMaterials = reinterpret_cast< uchar* >(vertexMaterialHeader + 1);
  uint* edgeMaterialHeader = reinterpret_cast< uint* >(vertexMaterials + *vertexMaterialHeader);
  uchar* edgeMaterials = reinterpret_cast< uchar* >(edgeMaterialHeader + 1);
  uint* surfaceMaterialHeader = reinterpret_cast< uint* >(edgeMaterials + *edgeMaterialHeader);
  uchar* surfaceMaterials = reinterpret_cast< uchar* >(surfaceMaterialHeader + 1);

  uint* edgeHeader = reinterpret_cast< uint* >(surfaceMaterials + *surfaceMaterialHeader);
  uint edgeCount = *edgeHeader;
  CCollisionEdge* edges = reinterpret_cast< CCollisionEdge* >(edgeHeader + 1);
  uint* surfaceHeader = reinterpret_cast< uint* >(edges + edgeCount);
  uint triangleCount = *surfaceHeader / 3;
  ushort* surfaceIndices = reinterpret_cast< ushort* >(surfaceHeader + 1);
  uint* extraIndexHeader = reinterpret_cast< uint* >(surfaceIndices + triangleCount * 3);
  ushort* extraIndices = reinterpret_cast< ushort* >(extraIndexHeader + 1);
  uint* vertexHeader = reinterpret_cast< uint* >(extraIndices + triangleCount * 3);
  uint vertexCount = *vertexHeader;
  CVector3f* vertices = reinterpret_cast< CVector3f* >(vertexHeader + 1);

  *treeOut = rs_new CAreaOctTree(bounds, treeType, static_cast< uchar* >(buffer), treeBuffer,
                                 materialCount, vertexCount, edgeCount, triangleCount, materials,
                                 vertexMaterials, edgeMaterials, surfaceMaterials, edges,
                                 surfaceIndices, extraIndices, vertices);
  *valid = true;
}
