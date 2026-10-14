#include "MetroidPrime/PathFinding/CPathFindArea.hpp"

#include "Kyoto/CFactoryMgr.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CloseEnough.hpp"
#include "Kyoto/SObjectTag.hpp"

#include "rstl/algorithm.hpp"

#include <dolphin/os.h>
#include <float.h>

#ifdef TARGET_PC
#include "Kyoto/Basics/CBasics.hpp"

#include <new>
#include <string.h>
#endif

class CVParamTransfer;

class CPFMemoryStream {
public:
  CPFMemoryStream(uchar* data, int size) : mData(data), mSize(size), mCurrent(data) {}
  int ReadInt32() {
    int value = *reinterpret_cast< int* >(mCurrent);
    mCurrent += sizeof(int);
    return value;
  }
  void* GetBlock(int count, int size) {
    void* block = mCurrent;
    mCurrent += count * size;
    return block;
  }

private:
  uchar* mData;
  int mSize;
  uchar* mCurrent;
};

// Guessed name; out-of-line global in the original, reads one word through the stream cursor.
void ReadStreamInt32(int* out, CPFMemoryStream* stream) { *out = stream->ReadInt32(); }

uint CPFAreaOctree::GetChildIndex(const CVector3f& point) const {
  uint index = 0;
  if (point[kDX] > mCenter[kDX]) {
    index = 1;
  }
  if (point[kDY] > mCenter[kDY]) {
    index |= 2;
  }
  if (point[kDZ] > mCenter[kDZ]) {
    index |= 4;
  }
  return index;
}

rstl::prereserved_vector< CPFRegion* >* CPFAreaOctree::GetRegionList(const CVector3f& point) {
  if (mIsLeaf) {
    return &mRegions;
  }
  return mChildren[GetChildIndex(point)]->GetRegionList(point);
}

inline void CPFAreaOctree::GetRegionListList(
    rstl::reserved_vector< rstl::prereserved_vector< CPFRegion* >*, 32 >& lists,
    const CVector3f& point, float padding) {
  if (lists.size() >= lists.capacity()) {
    return;
  }
  if (mIsLeaf) {
    lists.push_back(&mRegions);
  } else {
    for (int i = 0; i < 8; ++i) {
      if (mChildren[i]->IsPointInsidePaddedAABox(point, padding)) {
        mChildren[i]->GetRegionListList(lists, point, padding);
      }
    }
  }
}

CPFArea::CPFArea(const rstl::auto_ptr< uchar >& data, int size)
: mBestPointDistSq(FLT_MAX)
, mClosestPoint(CVector3f::Zero())
, mCachedRegionList(nullptr)
, mCachedRegionListPoint(CVector3f::Zero())
, mHasCachedRegionList(false)
, mRegionFindCookie(0)
, mVersion(-1)
, mData(data.release())
, mTransform(CTransform4f::Identity()) {
#ifdef TARGET_PC
  LoadHost();
#else
  int maxRegionNodes;
  CPFMemoryStream stream(mData.get(), size);
  int version;
  ReadStreamInt32(&version, &stream);
  mVersion = version;

  int numNodes = stream.ReadInt32();
  mNodes.set_size(numNodes);
  mNodes.set_data(static_cast< CPFNode* >(stream.GetBlock(numNodes, sizeof(CPFNode))));
  int numLinks = stream.ReadInt32();
  mLinks.set_size(numLinks);
  mLinks.set_data(static_cast< CPFLink* >(stream.GetBlock(numLinks, sizeof(CPFLink))));
  int numRegions = stream.ReadInt32();
  mRegions.set_size(numRegions);
  mRegions.set_data(static_cast< CPFRegion* >(stream.GetBlock(numRegions, sizeof(CPFRegion))));
  mRegionData.reserve(numRegions);
  CPFRegionData dataValue = CPFRegionData();
  mRegionData.resize(numRegions, dataValue);
  maxRegionNodes = 0;
  int i;
  for (i = 0; i < numRegions; ++i) {
    mRegions[i].Fixup(*this, maxRegionNodes);
  }
  maxRegionNodes = maxRegionNodes > 4 ? maxRegionNodes : 4;
  mPolyPoints.reserve(maxRegionNodes);

  uint numWords = (numRegions * (numRegions - 1) / 2 + 31) / 32;
  mConnectionsGround.set_size(numWords);
  mConnectionsGround.set_data(static_cast< uint* >(stream.GetBlock(numWords, sizeof(uint))));
  mConnectionsFlyers.set_size(numWords);
  mConnectionsFlyers.set_data(static_cast< uint* >(stream.GetBlock(numWords, sizeof(uint))));
  stream.GetBlock(((numRegions * numRegions + 31) / 32 - numWords) * 2, sizeof(uint));

  int numRegionPtrs = stream.ReadInt32();
  mOctreeRegions.set_size(numRegionPtrs);
  mOctreeRegions.set_data(
      static_cast< CPFRegion** >(stream.GetBlock(numRegionPtrs, sizeof(CPFRegion*))));
  for (i = 0; i < numRegionPtrs; ++i) {
    CPFRegion* const& region = mOctreeRegions[i];
    mOctreeRegions[i] = &mRegions[reinterpret_cast< intptr_t >(region)];
  }
  int numOctreeNodes = stream.ReadInt32();
  mOctree.set_size(numOctreeNodes);
  mOctree.set_data(
      static_cast< CPFAreaOctree* >(stream.GetBlock(numOctreeNodes, sizeof(CPFAreaOctree))));
  for (i = 0; i < numOctreeNodes; ++i) {
    mOctree[i].Fixup(*this);
  }
  if (mVersion > 4) {
    const int numPoints = stream.ReadInt32();
    if (numPoints > 0) {
      mPoints.set_size(numPoints);
      mPoints.set_data(static_cast< CPFPoint* >(stream.GetBlock(numPoints, sizeof(CPFPoint))));
      const int numPointLinks = stream.ReadInt32();
      mPointLinks.set_size(numPointLinks);
      mPointLinks.set_data(static_cast< int* >(stream.GetBlock(numPointLinks, sizeof(int))));
      const int numLinkData = stream.ReadInt32();
      mPointLinkCosts.set_size(numLinkData);
      mPointLinkCosts.set_data(static_cast< float* >(stream.GetBlock(numLinkData, sizeof(float))));
      const int numPointWords = (numPoints * (numPoints - 1) / 2 + 31) / 32;
      mPointConnections.set_size(numPointWords);
      mPointConnections.set_data(
          static_cast< uint* >(stream.GetBlock(numPointWords, sizeof(uint))));
      for (int j = 0; j < numPoints; ++j) {
        mPoints[j].Fixup(*this);
      }
    }
    mPointSearchState = rs_new CPFPointSearchState(numPoints);
  }
#endif
}

#ifdef TARGET_PC
static uint BigU32(const uchar* p) {
  return (uint(p[0]) << 24) | (uint(p[1]) << 16) | (uint(p[2]) << 8) | uint(p[3]);
}

static int BigS32(const uchar* p) { return static_cast< int >(BigU32(p)); }

static float BigF32(const uchar* p) {
  const uint bits = BigU32(p);
  float value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

static CVector3f BigVec(const uchar* p) {
  return CVector3f(BigF32(p), BigF32(p + 4), BigF32(p + 8));
}

static void SwapWords(uchar* p, int count) {
  uint* words = reinterpret_cast< uint* >(p);
  for (int i = 0; i < count; ++i) {
    words[i] = CBasics::SwapBytes(words[i]);
  }
}

template < typename T >
static T* IndexAsPointer(int index) {
  return reinterpret_cast< T* >(static_cast< intptr_t >(index));
}

// The disc PATH is laid out for in-place use on a 32-bit big-endian target: plain-data blocks
// are swapped in place in mData, and records holding pointers (regions, octree nodes, points,
// octree region lists) are rebuilt in host buffers. Pointer fields first receive their disc
// indices, which the regular Fixup calls then resolve.
void CPFArea::LoadHost() {
  uchar* cur = mData.get();
  mVersion = BigS32(cur);
  cur += 4;

  const int numNodes = BigS32(cur);
  cur += 4;
  SwapWords(cur, numNodes * 6);
  mNodes.set_size(numNodes);
  mNodes.set_data(reinterpret_cast< CPFNode* >(cur));
  cur += numNodes * 0x18;

  const int numLinks = BigS32(cur);
  cur += 4;
  SwapWords(cur, numLinks * 4);
  mLinks.set_size(numLinks);
  mLinks.set_data(reinterpret_cast< CPFLink* >(cur));
  cur += numLinks * 0x10;

  const int numRegions = BigS32(cur);
  cur += 4;
  mHostRegions = rs_new uchar[numRegions * sizeof(CPFRegion)];
  CPFRegion* regions = reinterpret_cast< CPFRegion* >(mHostRegions.get());
  for (int i = 0; i < numRegions; ++i, cur += 0x5c) {
    CPFRegion& region = *new (&regions[i]) CPFRegion();
    region.mNumNodes = BigS32(cur);
    region.mStartNode = IndexAsPointer< const CPFNode >(BigS32(cur + 4));
    region.mNumLinks = BigS32(cur + 8);
    region.mStartLink = IndexAsPointer< CPFLink >(BigS32(cur + 12));
    region.mFlags = BigU32(cur + 16);
    region.mHeight = BigF32(cur + 20);
    region.mNormal = BigVec(cur + 24);
    region.mRegionIdx = BigS32(cur + 36);
    region.mCentroid = BigVec(cur + 40);
    region.mBounds = CAABox(BigVec(cur + 52), BigVec(cur + 64));
    for (int k = 0; k < 3; ++k) {
      region.mObstructionCounts[k] = BigS32(cur + 76 + k * 4);
    }
  }
  mRegions.set_size(numRegions);
  mRegions.set_data(regions);
  mRegionData.reserve(numRegions);
  CPFRegionData dataValue = CPFRegionData();
  mRegionData.resize(numRegions, dataValue);
  int maxRegionNodes = 0;
  for (int i = 0; i < numRegions; ++i) {
    mRegions[i].Fixup(*this, maxRegionNodes);
  }
  maxRegionNodes = maxRegionNodes > 4 ? maxRegionNodes : 4;
  mPolyPoints.reserve(maxRegionNodes);

  const uint numWords = (numRegions * (numRegions - 1) / 2 + 31) / 32;
  SwapWords(cur, numWords * 2);
  mConnectionsGround.set_size(numWords);
  mConnectionsGround.set_data(reinterpret_cast< uint* >(cur));
  cur += numWords * 4;
  mConnectionsFlyers.set_size(numWords);
  mConnectionsFlyers.set_data(reinterpret_cast< uint* >(cur));
  cur += numWords * 4;
  cur += ((numRegions * numRegions + 31) / 32 - numWords) * 2 * 4;

  const int numRegionPtrs = BigS32(cur);
  cur += 4;
  mHostOctreeRegions = rs_new uchar[numRegionPtrs * sizeof(CPFRegion*)];
  CPFRegion** regionPtrs = reinterpret_cast< CPFRegion** >(mHostOctreeRegions.get());
  for (int i = 0; i < numRegionPtrs; ++i, cur += 4) {
    regionPtrs[i] = &mRegions[BigS32(cur)];
  }
  mOctreeRegions.set_size(numRegionPtrs);
  mOctreeRegions.set_data(regionPtrs);

  const int numOctreeNodes = BigS32(cur);
  cur += 4;
  mHostOctree = rs_new uchar[numOctreeNodes * sizeof(CPFAreaOctree)];
  memset(mHostOctree.get(), 0, numOctreeNodes * sizeof(CPFAreaOctree));
  CPFAreaOctree* octree = reinterpret_cast< CPFAreaOctree* >(mHostOctree.get());
  for (int i = 0; i < numOctreeNodes; ++i, cur += 0x50) {
    CPFAreaOctree& node = octree[i];
    node.mIsLeaf = BigU32(cur) != 0;
    node.mBounds = CAABox(BigVec(cur + 4), BigVec(cur + 16));
    node.mCenter = BigVec(cur + 28);
    for (int c = 0; c < 8; ++c) {
      node.mChildren[c] = IndexAsPointer< CPFAreaOctree >(BigS32(cur + 40 + c * 4));
    }
    node.mRegions.set_size(BigS32(cur + 72));
    node.mRegions.set_data(IndexAsPointer< CPFRegion* >(BigS32(cur + 76)));
  }
  mOctree.set_size(numOctreeNodes);
  mOctree.set_data(octree);
  for (int i = 0; i < numOctreeNodes; ++i) {
    mOctree[i].Fixup(*this);
  }

  if (mVersion > 4) {
    const int numPoints = BigS32(cur);
    cur += 4;
    if (numPoints > 0) {
      mHostPoints = rs_new uchar[numPoints * sizeof(CPFPoint)];
      CPFPoint* points = reinterpret_cast< CPFPoint* >(mHostPoints.get());
      for (int i = 0; i < numPoints; ++i, cur += 0x1c) {
        CPFPoint& point = points[i];
        point.mPosition = BigVec(cur);
        point.mFlags = BigU32(cur + 12);
        point.mNumLinks = BigU32(cur + 16);
        point.mLinks = IndexAsPointer< int >(BigS32(cur + 20));
        point.mLinkCosts = IndexAsPointer< float >(BigS32(cur + 24));
      }
      mPoints.set_size(numPoints);
      mPoints.set_data(points);

      const int numPointLinks = BigS32(cur);
      cur += 4;
      SwapWords(cur, numPointLinks);
      mPointLinks.set_size(numPointLinks);
      mPointLinks.set_data(reinterpret_cast< int* >(cur));
      cur += numPointLinks * 4;
      const int numLinkData = BigS32(cur);
      cur += 4;
      SwapWords(cur, numLinkData);
      mPointLinkCosts.set_size(numLinkData);
      mPointLinkCosts.set_data(reinterpret_cast< float* >(cur));
      cur += numLinkData * 4;
      const int numPointWords = (numPoints * (numPoints - 1) / 2 + 31) / 32;
      SwapWords(cur, numPointWords);
      mPointConnections.set_size(numPointWords);
      mPointConnections.set_data(reinterpret_cast< uint* >(cur));
      for (int j = 0; j < numPoints; ++j) {
        mPoints[j].Fixup(*this);
      }
    }
    mPointSearchState = rs_new CPFPointSearchState(numPoints);
  }
}
#endif

rstl::prereserved_vector< CPFRegion* >* CPFArea::GetOctreeRegionList(const CVector3f& point) {
  if (mHasCachedRegionList && close_enough(point, mCachedRegionListPoint)) {
    return mCachedRegionList;
  }
  return mOctree.back().GetRegionList(point);
}

int CPFArea::FindRegions(rstl::reserved_vector< CPFRegion*, 8 >& regions, const CVector3f& point,
                         uint flags, uint indexMask, bool ignoreObstructions) {
  rstl::prereserved_vector< CPFRegion* >* list = GetOctreeRegionList(point);
  for (int i = 0; i < list->size(); ++i) {
    CPFRegion* region = (*list)[i];
    if ((region->GetFlags() & 0xff & flags) && ((region->GetFlags() >> 16) & 0xff & indexMask) &&
        region->IsPointInside(point) &&
        (ignoreObstructions ||
         !(region->GetObstructionCount(kPFO_Unknown2) > 0 ||
           ((flags & 0x100) != 0 && region->GetObstructionCount(kPFO_Unknown0) > 0) ||
           ((flags & 0x200) != 0 && region->GetObstructionCount(kPFO_Unknown1) > 0))) &&
        ((flags & 6) || region->PointHeight(point) < 3.f)) {
      regions.push_back(region);
      if (regions.size() == regions.capacity()) {
        break;
      }
    }
  }
  return regions.size();
}

int CPFArea::FindRegions(rstl::reserved_vector< CPFRegion*, 8 >& regions, const CAABox& box,
                         uint flags, uint indexMask, bool ignoreObstructions) {
  for (int i = 0; i < mOctreeRegions.size(); ++i) {
    CPFRegion* region = mOctreeRegions[i];
    if ((region->GetFlags() & 0xff & flags) && ((region->GetFlags() >> 16) & 0xff & indexMask) &&
        region->Intersects(box) &&
        (ignoreObstructions ||
         !(region->GetObstructionCount(kPFO_Unknown2) > 0 ||
           ((flags & 0x100) != 0 && region->GetObstructionCount(kPFO_Unknown0) > 0) ||
           ((flags & 0x200) != 0 && region->GetObstructionCount(kPFO_Unknown1) > 0))) &&
        ((flags & 6) ||
         region->PointHeight(box.ClosestPointAlongVector(region->GetNormal())) < 3.f)) {
      regions.push_back(region);
      if (regions.size() == regions.capacity()) {
        break;
      }
    }
  }
  return regions.size();
}

CPFRegion* CPFArea::FindClosestRegion(const CVector3f& point, uint flags, uint indexMask,
                                      float padding) {
  rstl::reserved_vector< rstl::prereserved_vector< CPFRegion* >*, 32 > lists;
  CPFRegion* result = nullptr;
  OSGetTick();
  int i, j;
  uint searchTicks = 0;
  mOctree.back().GetRegionListList(lists, point, padding);
  OSGetTick();
  for (i = 0; i < lists.size(); ++i) {
    rstl::prereserved_vector< CPFRegion* >* list = lists[i];
    for (j = 0; j < list->size(); ++j) {
      CPFRegion* region = (*list)[j];
      if (region->Data()->GetCookie() != mRegionFindCookie) {
        region->Data()->SetCookie(mRegionFindCookie);
        if ((region->GetFlags() & 0xff & flags) &&
            ((region->GetFlags() >> 16) & 0xff & indexMask) &&
            !(region->GetObstructionCount(kPFO_Unknown2) > 0 ||
              ((flags & 0x100) != 0 && region->GetObstructionCount(kPFO_Unknown0) > 0) ||
              ((flags & 0x200) != 0 && region->GetObstructionCount(kPFO_Unknown1) > 0)) &&
            region->IsPointInsidePaddedAABox(point, padding)) {
          uint startTick = OSGetTick();
          if ((flags & 6) || region->PointHeight(point) < 3.f) {
            if (region->FindBestPoint(mPolyPoints, point, flags, padding * padding)) {
              padding = CMath::FastSqrtF(region->Data()->GetBestDistanceSquared());
              result = region;
              mClosestPoint = region->Data()->GetBestPoint();
            }
            searchTicks += OSGetTick() - startTick;
          }
        }
      }
    }
  }
  OSGetTick();
  ++mRegionFindCookie;
  return result;
}

CVector3f CPFArea::FindClosestReachablePoint(rstl::reserved_vector< CPFRegion*, 8 >& regions,
                                             const CVector3f& point, uint flags, uint indexMask) {
  CVector3f result = CVector3f::Zero();
  float closestDistanceSq = FLT_MAX;
  for (int i = 0; i < GetNumRegions(); ++i) {
    CPFRegion& region = GetRegion(i);
    if ((region.GetFlags() & 0xff & flags) && ((region.GetFlags() >> 16) & 0xff & indexMask) &&
        !(region.GetObstructionCount(kPFO_Unknown2) > 0 ||
          ((flags & 0x100) != 0 && region.GetObstructionCount(kPFO_Unknown0) > 0) ||
          ((flags & 0x200) != 0 && region.GetObstructionCount(kPFO_Unknown1) > 0))) {
      for (int j = 0; j < regions.size(); ++j) {
        CPFRegion* source = regions[j];
        if (PathExists(source, &region, flags)) {
          const CVector3f& delta = region.GetCentroid() - point;
          float distanceSq = delta.MagSquared();
          if (distanceSq < closestDistanceSq) {
            closestDistanceSq = distanceSq;
            result = region.GetCentroid();
            break;
          }
        }
      }
    }
  }
  return result;
}

bool CPFArea::PathExists(const CPFRegion* source, const CPFRegion* destination, uint flags) const {
  if (source == destination || (flags & 0x14)) {
    return true;
  }
  int numRegions = GetNumRegions();
  int sourceIndex = source->GetIndex();
  const int destIndex = destination->GetIndex();
  int destinationIndex = destIndex;
  const rstl::prereserved_vector< uint >& connections =
      (flags & 2) ? mConnectionsFlyers : mConnectionsGround;
  int lowIndex = sourceIndex;
  if (sourceIndex > destinationIndex) {
    lowIndex = destinationIndex;
    destinationIndex = sourceIndex;
  }
  int totalConnections = numRegions * (numRegions - 1) / 2;
  int remainingConnections = (numRegions - lowIndex - 1) * (numRegions - lowIndex) / 2;
  uint bit = totalConnections - remainingConnections + destinationIndex - (lowIndex + 1);
  return (connections[bit / 32] >> (bit % 32)) & 1;
}

void CPFArea::SetTransform(const CTransform4f& transform) {
  const CTransform4f delta = mTransform.GetInverse() * transform;
  for (int i = 0; i < mPoints.size(); ++i) {
    CPFPoint& point = mPoints[i];
    point.SetPosition(transform.GetTranslation() + delta.Rotate(point.GetPosition()));
  }
  mTransform = transform;
}

int CPFArea::GetPointIndex(const CPFPoint& point) const { return &point - &mPoints[0]; }

bool CPFArea::PointPathExists(int source, int destination) {
  if (source == destination) {
    return true;
  }

  const int count = mPoints.size();
  if (source > destination) {
    rstl::swap(source, destination);
  }
  const int totalConnections = count * (count - 1) / 2;
  const int remainingConnections = (count - source - 1) * (count - source) / 2;
  const uint bit = totalConnections - remainingConnections + destination - (source + 1);
  return (mPointConnections[bit / 32] >> (bit % 32)) & 1;
}

bool CPFArea::PointPathExists(const CPFPoint* source, const CPFPoint* destination) {
  if (!source || !destination) {
    return false;
  }
  if (source == destination) {
    return true;
  }
  return PointPathExists(source - mPoints.data(), destination - mPoints.data());
}

inline CPFArea::~CPFArea() {}

CFactoryFnReturn FPathFindAreaFactory(const SObjectTag& tag, const rstl::auto_ptr< uchar >& data,
                                      int size, const CVParamTransfer& xfer) {
  return rs_new CPFArea(data, size);
}
