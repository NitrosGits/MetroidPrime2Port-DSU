#include "MetroidPrime/CWorldSaveGameInfo.hpp"

#include "Kyoto/Alloc/CMemory.hpp"
#ifdef TARGET_PC
#include "Kyoto/Basics/CBasics.hpp"
#endif
#include "Kyoto/CCRC32.hpp"
#include "Kyoto/CFactoryMgr.hpp"
#include "Kyoto/Streams/CInputStream.hpp"

#include "rstl/single_ptr.hpp"

#include <string.h>

inline CWorldSaveGameInfo::SLayerState::SLayerState(CInputStream& in)
: mArea(in.ReadInt32()), mLayer(in.ReadInt32()) {}

CWorldSaveGameInfo::CWorldSaveGameInfo(CInputStream& in) : mAreaCount(0) {
  in.ReadInt32();
  const uint version = in.Get< uint >();
  mAreaCount = in.ReadInt32();
  mCinematics = rstl::vector< TEditorId >(in);
  mRelays = rstl::vector< TEditorId >(in);
  mLayers = rstl::vector< SLayerState >(in);
  mDoors = rstl::vector< TEditorId >(in);
  mScans = rstl::vector< ScanState >(in);
  if (version > 3) {
    mSystemVariables = rstl::vector< SEnvironmentVariable >(in);
    mGameVariables = rstl::vector< SEnvironmentVariable >(in);
  }
  if (version > 4) {
    mUnmappableObjects = rstl::vector< TEditorId >(in);
  }
}

int CWorldSaveGameInfo::GetRelayIndex(const TEditorId& id) const {
  for (int i = 0; i < mRelays.size(); ++i) {
    if (mRelays[i] == id) {
      return i;
    }
  }
  return -1;
}

#ifdef TARGET_PC
uint CWorldSaveGameInfo::CalculateHash(bool consoleOrder) const {
#else
uint CWorldSaveGameInfo::CalculateHash() const { // Guessed name
#endif
  int size = sizeof(mAreaCount);
  size += (mCinematics.size() + mRelays.size() + mDoors.size() + mUnmappableObjects.size()) *
          sizeof(TEditorId);
  size += mLayers.size() * sizeof(SLayerState);
  size += mScans.size() * sizeof(ScanState);
  rstl::single_ptr< uchar > buffer(rs_new uchar[size]);
  uchar* data = buffer.get();
  uint hash = 0;

  memcpy(data, &mAreaCount, sizeof(mAreaCount));
  data += sizeof(mAreaCount);
  if (mCinematics.size() > 0) {
    memcpy(data, mCinematics.data(), mCinematics.size() * sizeof(TEditorId));
    data += mCinematics.size() * sizeof(TEditorId);
  }
  if (mRelays.size() > 0) {
    memcpy(data, mRelays.data(), mRelays.size() * sizeof(TEditorId));
    data += mRelays.size() * sizeof(TEditorId);
  }
  if (mDoors.size() > 0) {
    memcpy(data, mDoors.data(), mDoors.size() * sizeof(TEditorId));
    data += mDoors.size() * sizeof(TEditorId);
  }
  if (mLayers.size() > 0) {
    memcpy(data, mLayers.data(), mLayers.size() * sizeof(SLayerState));
    data += mLayers.size() * sizeof(SLayerState);
  }
  if (mScans.size() > 0) {
    memcpy(data, mScans.data(), mScans.size() * sizeof(ScanState));
    data += mScans.size() * sizeof(ScanState);
  }
  if (mUnmappableObjects.size() > 0) {
    memcpy(data, mUnmappableObjects.data(), mUnmappableObjects.size() * sizeof(TEditorId));
  }

  for (rstl::vector< SEnvironmentVariable >::const_iterator it = mSystemVariables.begin();
       it != mSystemVariables.end(); ++it) {
    hash = CCRC32::CalculateString(it->mName.data(), hash);
  }
  for (rstl::vector< SEnvironmentVariable >::const_iterator it = mGameVariables.begin();
       it != mGameVariables.end(); ++it) {
    hash = CCRC32::CalculateString(it->mName.data(), hash);
  }

#ifdef TARGET_PC
  // The buffer is all 32-bit words; the console hashed them big-endian.
  static_assert(sizeof(TEditorId) == 4 && sizeof(SLayerState) == 8 && sizeof(ScanState) == 8,
                "CalculateHash assumes a buffer of 32-bit words");
  if (consoleOrder) {
    uint* words = reinterpret_cast< uint* >(buffer.get());
    for (int i = 0; i < size / 4; ++i) {
      words[i] = CBasics::SwapBytes(words[i]);
    }
  }
#endif
  return CCRC32::Calculate(buffer.get(), size, hash);
}

CFactoryFnReturn FSaveWorldFactory(const SObjectTag& tag, CInputStream& in,
                                   const CVParamTransfer& params) {
  return rs_new CWorldSaveGameInfo(in);
}
