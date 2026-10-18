#include "Kyoto/Audio/CSfxHandle.hpp"
#include "Kyoto/Audio/CMidiManager.hpp"

#include "Kyoto/Audio/CAudioSys.hpp"

#include "Kyoto/CFactoryMgr.hpp"
#include "Kyoto/Streams/CInputStream.hpp"
#ifdef TARGET_PC

#include "Kyoto/Basics/CBasics.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A CSNG's song (MusyX ARR) data is authored big-endian and MusyX reads it through native structs,
// so convert it in place once, when the resource loads (MP1's port does the same). Without this the
// sequencer reads byte-swapped offsets and tempos, and no world music plays although SFX do.
namespace {
bool DebugAudio() {
  static const bool enabled = getenv("MP2_DEBUG_AUDIO") != nullptr;
  return enabled;
}

uint ReadBig32(const uchar* p) {
  uint v;
  memcpy(&v, p, sizeof(v));
  return CBasics::SwapBytes(v);
}
ushort ReadBig16(const uchar* p) {
  ushort v;
  memcpy(&v, p, sizeof(v));
  return CBasics::SwapBytes(v);
}
void Swap32At(uchar* p) {
  uint v = ReadBig32(p);
  memcpy(p, &v, sizeof(v));
}
void Swap16At(uchar* p) {
  ushort v = ReadBig16(p);
  memcpy(p, &v, sizeof(v));
}

// Offsets already converted: tracks or patterns shared between entries must be swapped once.
struct SSeen {
  uint mOffsets[256];
  int mCount;
  bool Visit(uint offset) {
    for (int i = 0; i < mCount; ++i) {
      if (mOffsets[i] == offset) {
        return false;
      }
    }
    if (mCount < 256) {
      mOffsets[mCount++] = offset;
    }
    return true;
  }
};

// TENTRY {u32 time; u8 prg, vel, res[2]; u16 pattern; s8 transpose, velAdd}, ended by pattern
// 0xffff; pattern 0xfffe is a loop whose target index is the u16 at +0xa.
void ConvertTrack(uchar* data, uint size, uint offset, uint& maxPattern) {
  for (; offset + 0xc <= size; offset += 0xc) {
    uchar* e = data + offset;
    Swap32At(e);
    Swap16At(e + 8);
    ushort pattern;
    memcpy(&pattern, e + 8, sizeof(pattern));
    if (pattern == 0xffff) {
      return;
    }
    if (pattern == 0xfffe) {
      Swap16At(e + 0xa);
    } else if (pattern > maxPattern) {
      maxPattern = pattern;
    }
  }
}

// NOTE_DATA {u16 time; u8 key, velocity; u16 length}; a controller event (key & 0x80) or an empty
// one is 4 bytes, a note 6; key == velocity == 0xff ends the pattern.
void ConvertNotes(uchar* data, uint size, uint offset) {
  while (offset + 4 <= size) {
    uchar* e = data + offset;
    Swap16At(e);
    const uchar key = e[2];
    const uchar velocity = e[3];
    if (key == 0xff && velocity == 0xff) {
      return;
    }
    if ((key & 0x80) != 0 || (key | velocity) == 0) {
      offset += 4;
      continue;
    }
    if (offset + 6 > size) {
      return;
    }
    Swap16At(e + 4);
    offset += 6;
  }
}

// ARR {u32 tTab, pTab, tmTab, mTrack, info, loopPoint[16], tsTab} (0x58).
void ConvertArrData(uchar* data, uint size) {
  if (size < 0x58) {
    return;
  }
  for (uint off = 0; off < 0x58; off += 4) {
    Swap32At(data + off);
  }
  uint tTab, pTab, mTrack;
  memcpy(&tTab, data + 0x0, 4);
  memcpy(&pTab, data + 0x4, 4);
  memcpy(&mTrack, data + 0xc, 4);

  SSeen seen;
  seen.mCount = 0;
  uint maxPattern = 0;
  bool hasTracks = false;
  if (tTab + 64 * 4 <= size) {
    for (uint i = 0; i < 64; ++i) {
      Swap32At(data + tTab + 4 * i);
      uint track;
      memcpy(&track, data + tTab + 4 * i, 4);
      if (track != 0 && track + 0xc <= size && seen.Visit(track)) {
        ConvertTrack(data, size, track, maxPattern);
        hasTracks = true;
      }
    }
  }

  // MTRACK_DATA {u32 time, bpm}, ended by time 0xffffffff.
  if (mTrack != 0) {
    for (uint off = mTrack; off + 8 <= size; off += 8) {
      Swap32At(data + off);
      Swap32At(data + off + 4);
      uint time;
      memcpy(&time, data + off, 4);
      if (time == 0xffffffff) {
        break;
      }
    }
  }

  // SEQ_PATTERN {u32 headerLen, pitchBend, modulation} then the note data. The pitch-bend and
  // modulation streams are bytes.
  if (hasTracks && pTab + 4 * (maxPattern + 1) <= size) {
    seen.mCount = 0;
    for (uint i = 0; i <= maxPattern; ++i) {
      Swap32At(data + pTab + 4 * i);
      uint pattern;
      memcpy(&pattern, data + pTab + 4 * i, 4);
      if (pattern == 0 || pattern + 0xc > size || !seen.Visit(pattern)) {
        continue;
      }
      Swap32At(data + pattern);
      Swap32At(data + pattern + 4);
      Swap32At(data + pattern + 8);
      ConvertNotes(data, size, pattern + 0xc);
    }
  }
}
} // namespace
#endif

rstl::reserved_vector< CMidiManager::CMidiWrapper, 3 > CMidiManager::mMidiWrappers;

CMidiManager::CMidiWrapper::CMidiWrapper() : mSysHandle(0), mAvailable(true) {}

const CSfxHandle& CMidiManager::CMidiWrapper::GetManagerHandle() const { return mMidiHandle; }

const uint CMidiManager::CMidiWrapper::GetAudioSysHandle() const { return mSysHandle; }

const bool CMidiManager::CMidiWrapper::IsAvailable() const { return mAvailable; }

const short CMidiManager::CMidiWrapper::GetSongId() const { return mSongId; }

void CMidiManager::CMidiWrapper::SetAvailable(const bool v) { mAvailable = v; }

void CMidiManager::CMidiWrapper::SetAudioSysHandle(const uint handle) { mSysHandle = handle; }

void CMidiManager::CMidiWrapper::SetMidiHandle(const CSfxHandle& handle) { mMidiHandle = handle; }

void CMidiManager::CMidiWrapper::SetSongId(const short id) { mSongId = id; }

CSfxHandle CMidiManager::Play(const CMidiData& data, unsigned short fadeTime, bool stopExisting,
                              short volume) {
  bool foundExisting = false;
  uint sysHandle = 0;
  CSfxHandle handle = LocateHandle();
  if (!handle) {
    return CSfxHandle();
  }
  CMidiWrapper& wrapper = mMidiWrappers[handle.GetIndex()];
  wrapper.SetAvailable(false);
  wrapper.SetMidiHandle(handle);
  if (stopExisting) {
    for (int i = 0; i < mMidiWrappers.size(); ++i) {
      if (mMidiWrappers[i].IsAvailable()) {
        continue;
      }

      if (data.GetSongId() == mMidiWrappers[i].GetSongId()) {
        foundExisting = true;
        sysHandle = mMidiWrappers[i].GetAudioSysHandle();
        mMidiWrappers[i].SetAvailable(true);
      } else {
        Stop(mMidiWrappers[i].GetManagerHandle(), fadeTime);
      }
    }
  }

  if (foundExisting) {
    wrapper.SetAudioSysHandle(sysHandle);
    wrapper.SetSongId(data.GetSongId());
  } else {
    uint sysHandle = CAudioSys::SeqPlayEx(data.GetGroupId(), data.GetSongId(), data.GetData(), nullptr, 0);
#ifdef TARGET_PC
    if (DebugAudio()) {
      fprintf(stderr, "[audio] midi play group=%d song=%d vol=%d fade=%u -> seq=%u\n",
              data.GetGroupId(), data.GetSongId(), volume, fadeTime, sysHandle);
    }
#endif
    if (fadeTime != 0) {
      CAudioSys::SeqVolume(0, 0, sysHandle, 0);
    }
    CAudioSys::SeqVolume(volume, fadeTime, sysHandle, 0);
    wrapper.SetAudioSysHandle(sysHandle);
    wrapper.SetSongId(data.GetSongId());
  }

  return handle;
}

void CMidiManager::Stop(const CSfxHandle& handle, ushort fadeTime) {
  if (!handle) {
    return;
  }

  if (handle != mMidiWrappers[handle.GetIndex()].GetManagerHandle()) {
    return;
  }

  uint sysHandle = mMidiWrappers[handle.GetIndex()].GetAudioSysHandle();
  if (fadeTime == 0) {
    CAudioSys::SeqStop(sysHandle);
  } else {
    CAudioSys::SeqVolume(0, fadeTime, sysHandle, 1);
  }

  mMidiWrappers[handle.GetIndex()].SetAvailable(true);
}

void CMidiManager::StopAll() {
  for (int i = 0; i < mMidiWrappers.size(); ++i) {
    if (!mMidiWrappers[i].IsAvailable()) {
      Stop(mMidiWrappers[i].GetManagerHandle(), 0);
    }
  }
}

CSfxHandle CMidiManager::LocateHandle() {
  for (int i = 0; i < mMidiWrappers.size(); ++i) {
    if (mMidiWrappers[i].IsAvailable()) {
      return CSfxHandle(i);
    }
  }

  if (mMidiWrappers.size() == mMidiWrappers.capacity()) {
    return CSfxHandle();
  }

  mMidiWrappers.push_back(CMidiWrapper());
  return CSfxHandle(mMidiWrappers.size() - 1);
}

CMidiManager::CMidiData::CMidiData(CInputStream& in)
: mSongId(-1), mGroupId(-1), mAgscId(-1) {
  in.ReadInt32();
  mSongId = in.ReadInt32();
  mGroupId = in.ReadInt32();
  mAgscId = in.ReadInt32();
  int len = in.ReadInt32();
  mData = rs_new uchar[len];
  in.Get(mData.get(), len);
#ifdef TARGET_PC
  if (DebugAudio()) {
    fprintf(stderr, "[audio] CSNG song=%d group=%d agsc=%d len=%d raw tTab=%08x info=%08x\n", mSongId,
            mGroupId, mAgscId, len, len >= 4 ? ReadBig32(mData.get()) : 0,
            len >= 0x14 ? ReadBig32(mData.get() + 0x10) : 0);
  }
  ConvertArrData(mData.get(), static_cast< uint >(len));
#endif
}

CFactoryFnReturn FMidiDataFactory(const SObjectTag& tag, CInputStream& in, const CVParamTransfer&) {
  return rs_new CMidiManager::CMidiData(in);
}
