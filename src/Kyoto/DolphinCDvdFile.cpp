#include "Kyoto/CDvdFile.hpp"
#include "Kyoto/CDvdRequest.hpp"

#include "Kyoto/CARAMManager.hpp"

#include "rstl/math.hpp"

#include "dolphin/os/OSCache.h"

#include "dolphin/arq.h"
#include "dolphin/dvd.h"
#include "dolphin/os.h"

#include "string.h"

#ifdef TARGET_PC
#include <new>
#include <stdio.h>
#endif

static CDvdFile* sFirstARAM = nullptr;
// The original names of these two mode/activity flags are not yet known.
#ifdef TARGET_PC
// Polling mode: the host's ARAM transfers are not driven by interrupts. Retail's own answer to
// that is this flag, and in it only the game thread touches the transfer state; the worker
// writes one `bool`.
bool lbl_80419B9C = true;
#else
bool lbl_80419B9C = false;
#endif
bool lbl_80419B9D = false;

struct CDvdFileARAM {
  CDvdFileARAM()
  : mGotARAMInterrupt(false)
  , mGotDvdInterrupt(false)
  , mFileSize1(0)
  , mFileSize2(0)
  , mCurBufferLen(0)
  , mAramOffset(0)
  , mBufferLen(0)
  , mBufferIndex(0) {}
  ~CDvdFileARAM() {}

  ARQRequest mARQRequest;
  struct SDvdInfo {
    SDvdInfo() : mDvdFile(nullptr), mNextfile(nullptr) {}
    DVDFileInfo mDvdFileInfo;
    CDvdFile* mDvdFile;
    CDvdFile* mNextfile;
  } mInfo;
  rstl::reserved_vector< rstl::auto_ptr< uchar >, 2 > mBuffers;
  bool mGotARAMInterrupt;
  bool mGotDvdInterrupt;
  uint mFileSize1;
  uint mFileSize2;
  int mCurBufferLen;
  uint mAramOffset;
  int mBufferLen;
  uint mBufferIndex;
};

CHECK_SIZEOF(CDvdFileARAM, 0x94)

const char* DecodeARAMFile(const char* filename) {
  if (!strncmp(filename, "aram:", 5)) {
    return filename + 5;
  }

  return filename;
}

void CDvdFile::DVDARAMXferCallback(s32 result, DVDFileInfo* info) {
  CDvdFileARAM::SDvdInfo* ptr = reinterpret_cast< CDvdFileARAM::SDvdInfo* >(info);
  if (!lbl_80419B9C) {
    DVDClose(&ptr->mDvdFileInfo);
  }
  ptr->mDvdFile->HandleDVDInterrupt();
}

void CDvdFile::ARAMARAMXferCallback(u32 addr) {
  reinterpret_cast< CDvdFileARAM* >(addr)->mInfo.mDvdFile->HandleARAMInterrupt();
}

#ifdef TARGET_PC
namespace {
// Port: the console stores ARQ payloads in a u32 address; Aurora carries a host
// pointer. The adapter keeps the console signature (and therefore the matching
// build) intact while the port passes a real pointer.
//
// It does `ARAMARAMXferCallback`'s one line itself rather than forwarding to it, because
// truncating the `CDvdFileARAM*` - a 64-bit heap pointer - to u32 would cut it to its low 32 bits.
void PortARAMARAMXferCallback(uintptr_t addr) {
  reinterpret_cast< CDvdFileARAM* >(addr)->mInfo.mDvdFile->HandleARAMInterrupt();
}
} // namespace
#define ARAMARAM_XFER_CALLBACK PortARAMARAMXferCallback
#else
#define ARAMARAM_XFER_CALLBACK ARAMARAMXferCallback
#endif

void CDvdFile::HandleARAMInterrupt() {
  BOOL enabled = OSDisableInterrupts();
  CDvdFileARAM* arFile = mARAMFile.get();

  arFile->mGotARAMInterrupt = true;

  if (!lbl_80419B9C && arFile->mGotARAMInterrupt && arFile->mGotDvdInterrupt) {
    PingARAMTransfer();
  }

  OSRestoreInterrupts(enabled);
}

void CDvdFile::HandleDVDInterrupt() {
  BOOL enabled = OSDisableInterrupts();
  CDvdFileARAM* arFile = mARAMFile.get();

  arFile->mGotDvdInterrupt = true;

  if (!lbl_80419B9C && arFile->mGotARAMInterrupt && arFile->mGotDvdInterrupt) {
    PingARAMTransfer();
  }

  OSRestoreInterrupts(enabled);
}

void CDvdFile::PingARAMTransfer() {
  CDvdFileARAM* aramFile = mARAMFile.get();

  if (aramFile->mBufferLen == 0) {
    PopARAMFileLoad();
    return;
  }

  int length = rstl::min_val(65536, aramFile->mBufferLen);
#ifdef TARGET_PC
  // Port: Aurora's ARQ takes pointer-width addresses, so hand it the host
  // pointers rather than the console's 32-bit encoding.
  ARQPostRequest(&aramFile->mARQRequest, 0, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
                 reinterpret_cast< uintptr_t >(aramFile->mBuffers[aramFile->mBufferIndex].get()),
                 reinterpret_cast< uintptr_t >(mARAMBuffer + aramFile->mAramOffset), length,
                 ARAMARAM_XFER_CALLBACK);
#else
  ARQPostRequest(&aramFile->mARQRequest, 0, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
                 reinterpret_cast< u32 >(aramFile->mBuffers[aramFile->mBufferIndex].get()),
                 reinterpret_cast< u32 >(mARAMBuffer + aramFile->mAramOffset), length,
                 ARAMARAM_XFER_CALLBACK);
#endif

  aramFile->mBufferLen -= length;
  aramFile->mAramOffset += length;
  aramFile->mGotARAMInterrupt = false;
  aramFile->mBufferIndex ^= 1;

  if (aramFile->mCurBufferLen != 0) {
    int length2 = rstl::min_val(65536, aramFile->mCurBufferLen);
    aramFile->mGotDvdInterrupt = false;
    DVDFastOpen(mFileEntry, &aramFile->mInfo.mDvdFileInfo);
    DVDReadAsync(&aramFile->mInfo.mDvdFileInfo, aramFile->mBuffers[aramFile->mBufferIndex].get(),
                 length2, aramFile->mFileSize2, DVDARAMXferCallback);
    lbl_80419B9D = true;
    aramFile->mFileSize2 += length2;
    aramFile->mCurBufferLen -= length2;
  }
}

void CDvdFile::TryARAMFile() {
  mARAMBuffer = static_cast< uchar* >(CARAMManager::Alloc(mSize));
  if (!CARAMManager::IsAllocValid(mARAMBuffer)) {
#ifdef TARGET_PC
    printf("[aram] TryARAMFile(%s): CARAMManager::Alloc(%d) FAILED - this pak will not be "
           "staged, and IsARAMFileLoaded will answer true from the start.\n",
           mFilename.data(), mSize);
    fflush(nullptr);
#endif
    return;
  }
  mARAMFile = rs_new CDvdFileARAM();
  CDvdFileARAM* arfile = mARAMFile.get();
  arfile->mInfo.mDvdFile = this;
  arfile->mGotARAMInterrupt = true;
  arfile->mFileSize1 = arfile->mCurBufferLen = arfile->mBufferLen = GetFileSize();
  mARAMAllocated = true;
#ifdef TARGET_PC
  printf("[aram] TryARAMFile(%s): alloc %p ok, %d bytes; queued for staging.\n",
         mFilename.data(), static_cast< void* >(mARAMBuffer), mSize);
  fflush(nullptr);
#endif
  PushARAMFileLoad();
}

void CDvdFile::PushARAMFileLoad() {
  BOOL enabled = true;
  if (!lbl_80419B9C) {
    enabled = OSDisableInterrupts();
  }
  CDvdFile* file = sFirstARAM;
  if (file == NULL) {
    sFirstARAM = this;
    StartARAMFileLoad();
  } else {
    for (CDvdFile* p = file; p != nullptr; p = p->mARAMFile->mInfo.mNextfile) {
      if (p->mARAMFile->mInfo.mNextfile == nullptr) {
        p->mARAMFile->mInfo.mNextfile = this;
        break;
      }
    }
  }
  if (!lbl_80419B9C) {
    OSRestoreInterrupts(enabled);
  }
}

void CDvdFile::PopARAMFileLoad() {
  BOOL enabled = true;
  if (!lbl_80419B9C) {
    enabled = OSDisableInterrupts();
  }
  CDvdFile* file = mARAMFile->mInfo.mNextfile;
  mARAMPopped = true;
  sFirstARAM = file;
  if (file != nullptr) {
    file->StartARAMFileLoad();
  }

  if (!lbl_80419B9C) {
    OSRestoreInterrupts(enabled);
  }
}

bool CDvdFile::IsARAMFileLoaded() {
  if (!mARAMAllocated) {
    return true;
  }

#ifdef TARGET_PC
  // HOST DIAGNOSTIC. One line per *change* of the staging state, because this is polled from
  // the pak pump thousands of times and the interesting fact is that the two interrupt flags
  // never both come up (or come up and never drain). `mBufferLen`/`mCurBufferLen` are the
  // two counters: bytes still to push to ARAM and bytes still to read off the disc.
  {
    static const CDvdFile* sLast = nullptr;
    static int sLastKey = -1;
    const int key = (mARAMPopped ? 8 : 0) |
                    ((mARAMFile.get() != nullptr && mARAMFile->mGotARAMInterrupt) ? 4 : 0) |
                    ((mARAMFile.get() != nullptr && mARAMFile->mGotDvdInterrupt) ? 2 : 0) |
                    (mARAMFile.get() == nullptr ? 1 : 0);
    if (sLast != this || sLastKey != key) {
      sLast = this;
      sLastKey = key;
      CDvdFileARAM* ar = mARAMFile.get();
      printf("[aram] %s: popped=%d aramIRQ=%d dvdIRQ=%d bufferLen=%d curBufferLen=%d "
             "fileSize2=%u -> loaded=%d\n",
             mFilename.data(), static_cast< int >(mARAMPopped),
             ar != nullptr && ar->mGotARAMInterrupt, ar != nullptr && ar->mGotDvdInterrupt,
             ar != nullptr ? ar->mBufferLen : -1, ar != nullptr ? ar->mCurBufferLen : -1,
             ar != nullptr ? ar->mFileSize2 : 0u,
             (!mARAMPopped ? 0 : 1));
      fflush(nullptr);
    }
  }
#endif

  if (!mARAMPopped) {
#ifdef TARGET_PC
    // Aurora delivers ARQ completions only from `ARQPoll()`; this is where polling mode waits
    // for them, so this is where they are delivered.
    ARQPoll();
    if (lbl_80419B9C && mARAMFile->mGotARAMInterrupt && mARAMFile->mGotDvdInterrupt) {
      // Polling mode skips `DVDARAMXferCallback`'s `DVDClose`, and `PingARAMTransfer`'s
      // `DVDFastOpen` then zeroes the file info - which on Aurora drops the open handle on the
      // floor, one per 64 KB. The read has completed (that is what the flag says), so close it
      // here, on this thread. Closing an already-closed info is a no-op.
      DVDClose(&mARAMFile->mInfo.mDvdFileInfo);
      PingARAMTransfer();
    }
#else
    if (lbl_80419B9C && mARAMFile->mGotARAMInterrupt && mARAMFile->mGotDvdInterrupt) {
      PingARAMTransfer();
    }
#endif
    return false;
  }

  mARAMFile = nullptr;

  return true;
}

void CDvdFile::StartARAMFileLoad() {
  CDvdFileARAM* aramFile = mARAMFile.get();
#ifdef TARGET_PC
  // Port: the two buffers are owned by `auto_ptr< uchar >`, whose destructor is `delete`. Under
  // mwcceppc `delete` is `CMemory::Free` (Kyoto/Alloc/CMemory.hpp), so retail's `CMemory::Alloc`
  // pairs with it; on the host `delete` is the C++ runtime's, and handing it a game-heap block
  // aborts in glibc (`munmap_chunk(): invalid pointer`) the moment the first `aram:` pak finishes
  // staging and `IsARAMFileLoaded` drops its `CDvdFileARAM`. So allocate them from the runtime.
  // 32-byte alignment is retail's (the game heap's) and Aurora's `DVDReadAsync` asserts it; the
  // aligned global `operator new` is `aligned_alloc` on glibc, which `delete` frees correctly.
  aramFile->mBuffers.push_back(static_cast< uchar* >(::operator new(0x10000, std::align_val_t(32))));
  aramFile->mBuffers.push_back(static_cast< uchar* >(::operator new(0x10000, std::align_val_t(32))));
#else
  aramFile->mBuffers.push_back(
      static_cast< uchar* >(CMemory::Alloc(0x10000, IAllocator::kHI_RoundUpLen)));
  aramFile->mBuffers.push_back(
      static_cast< uchar* >(CMemory::Alloc(0x10000, IAllocator::kHI_RoundUpLen)));
#endif

  int len = rstl::min_val(mSize, 65536);
  aramFile->mCurBufferLen -= len;
  aramFile->mFileSize2 = len;
  if (!lbl_80419B9C) {
    DVDFastOpen(mFileEntry, &aramFile->mInfo.mDvdFileInfo);
  } else {
#ifdef TARGET_PC
    const BOOL opened = DVDOpen(const_cast< char* >(DecodeARAMFile(mFilename.data())),
                                &aramFile->mInfo.mDvdFileInfo);
    printf("[aram] StartARAMFileLoad(%s): DVDOpen(\"%s\") -> %d, first read %d bytes at 0, "
           "buf0=%p align32=%d\n",
           mFilename.data(), DecodeARAMFile(mFilename.data()), static_cast< int >(opened), len,
           aramFile->mBuffers[0].get(),
           (reinterpret_cast< uintptr_t >(aramFile->mBuffers[0].get()) & 31) == 0);
    fflush(nullptr);
#else
    DVDOpen(const_cast< char* >(DecodeARAMFile(mFilename.data())), &aramFile->mInfo.mDvdFileInfo);
#endif
  }
  DVDReadAsync(&aramFile->mInfo.mDvdFileInfo, aramFile->mBuffers[0].get(), len, 0,
               DVDARAMXferCallback);
  lbl_80419B9D = true;
#ifdef TARGET_PC
  printf("[aram] StartARAMFileLoad(%s): DVDReadAsync issued, cb.state=%d userData=%p\n",
         mFilename.data(), static_cast< int >(aramFile->mInfo.mDvdFileInfo.cb.state),
         aramFile->mInfo.mDvdFileInfo.cb.userData);
  fflush(nullptr);
#endif
}

void CDvdFile::StallForARAMFile() {
  while (mARAMFile.get() != nullptr) {
    OSYieldThread();
  }
}

CDvdFile::CDvdFile(const char* filename)
: mFileEntry(-1)
, mARAMBuffer(0)
, mARAMAllocated(false)
, mARAMPopped(false)
, mARAMFile(nullptr)
, mOffset(0)
, mSize(0)
, mFilename(filename, -1) {
  const char* decodedName = DecodeARAMFile(filename);
  mFileEntry = DVDConvertPathToEntrynum(const_cast< char* >(decodedName));
  DVDFileInfo fileInfo;
  if (mFileEntry != -1) {
    DVDFastOpen(mFileEntry, &fileInfo);
  }

  mSize = fileInfo.length;
  DVDClose(&fileInfo);

  if (filename != decodedName) {
    TryARAMFile();
  }
}

CDvdFile::~CDvdFile() { CloseFile(); }

CDvdRequest* CDvdFile::SyncRead(void* dest, uint len) {
  return AsyncSeekRead(dest, len, kSO_Cur, 0);
}

void CDvdFile::SyncSeekRead(void* dest, uint len, ESeekOrigin origin, int offset) {
  StallForARAMFile();
  CalcFileOffset(offset, origin);

  if (mARAMAllocated) {
    uint roundedLen = (len + 31) & ~31;
    DCFlushRange(dest, roundedLen);
    CARAMManager::WaitForDMACompletion(CARAMManager::DMAToMRAM(
        mARAMBuffer + mOffset, dest, roundedLen, CARAMManager::kDMAPrio_One));
  } else {
    DVDFileInfo info;
    if (!lbl_80419B9C) {
      DVDFastOpen(mFileEntry, &info);
    } else {
      DVDOpen(const_cast< char* >(DecodeARAMFile(mFilename.data())), &info);
    }
    DVDReadAsync(&info, dest, (len + 31) & ~31, mOffset, internalCallback);
    lbl_80419B9D = true;
    while (DVDGetCommandBlockStatus(&info.cb) != DVD_STATE_END) {
    }
    DVDClose(&info);
  }

  UpdateFilePos(len);
}

CDvdRequest* CDvdFile::AsyncSeekRead(void* dest, uint len, ESeekOrigin origin, int offset) {
  StallForARAMFile();
  CalcFileOffset(offset, origin);
  CDvdRequest* request;
  if (mARAMAllocated) {
    const int roundedLen = (len + 31) & ~31;
    DCFlushRange(dest, roundedLen);
    request = rs_new CARAMDvdRequest(CARAMManager::DMAToMRAM(
        mARAMBuffer + mOffset, dest, roundedLen, CARAMManager::kDMAPrio_One));
#ifdef TARGET_PC
    printf("[dvd] AsyncSeekRead(%s): ARAM path, len=%u off=%d -> CARAMDvdRequest %p\n",
           mFilename.data(), len, mOffset, static_cast< void* >(request));
    fflush(nullptr);
#endif
  } else {
    CRealDvdRequest* req = rs_new CRealDvdRequest();
    DVDFileInfo* info = &req->FileInfo();
#ifdef TARGET_PC
    // HOST DIAGNOSTIC. `DVDFastOpen` returns FALSE **without touching `fileInfo`** when the
    // entrynum is not valid, so a failed open leaves `cb.state` as whatever `rs_new`'s raw
    // memory held and `IsComplete` then polls a block that was never written. The buffer's
    // 32-byte alignment is printed alongside it because Aurora asserts it in
    // `DVDReadAbsAsyncPrioInternal` (dvd.cpp:743).
    const BOOL fastOpen = DVDFastOpen(mFileEntry, info);
    printf("[dvd] AsyncSeekRead(%s): DVD path, entry=%d len=%u off=%d roundLen=%u "
           "fastOpen=%d buf=%p align32=%d\n",
           mFilename.data(), mFileEntry, len, mOffset, (len + 31) & ~31,
           static_cast< int >(fastOpen), dest,
           (reinterpret_cast< uintptr_t >(dest) & 31) == 0);
    fflush(nullptr);
#else
    DVDFastOpen(mFileEntry, info);
#endif
    DVDReadAsync(info, dest, (len + 31) & ~31, mOffset, internalCallback);
    lbl_80419B9D = true;
    request = req;
#ifdef TARGET_PC
    printf("[dvd] AsyncSeekRead(%s): issued; cb.state=%d userData=%p\n", mFilename.data(),
           static_cast< int >(info->cb.state), info->cb.userData);
    fflush(nullptr);
#endif
  }

  UpdateFilePos(len);

  return request;
}

void CDvdFile::CloseFile() {
  if (!mARAMAllocated) {
    return;
  }

  StallForARAMFile();
  CARAMManager::Free(mARAMBuffer);
}

#ifdef TARGET_PC
// HOST DIAGNOSTIC: the boot log says, per call, what the real disc answered. mwcceppc does not
// define `TARGET_PC`, so the matching build is unchanged.
bool CDvdFile::FileExists(const char* filename) {
  const char* const decodedName = DecodeARAMFile(filename);
  const s32 entry = DVDConvertPathToEntrynum(const_cast< char* >(decodedName));
  const bool exists = entry != -1;
  // The size, because "exists" alone cannot distinguish a pak from a directory or a truncated
  // read, and `DVDFastOpen` is what actually hands the entry to the reader. Printed only on a
  // hit: a miss has no size and the name alone is the whole answer.
  u32 length = 0;
  if (exists) {
    DVDFileInfo info;
    if (DVDFastOpen(entry, &info)) {
      length = info.length;
      DVDClose(&info);
    }
  }
  if (exists) {
    printf("[dvd] FileExists(\"%s\") -> true, %u bytes (entry %d)\n", filename, length, entry);
    if (length == 0) {
      printf("[dvd]   ^ non-zero entry but zero length: the FST entry resolved to nothing "
             "readable, which is a disc problem, not a lookup problem.\n");
    }
  } else {
    printf("[dvd] FileExists(\"%s\") -> false\n", filename);
  }
  return exists;
}
#else
bool CDvdFile::FileExists(const char* filename) {
  return DVDConvertPathToEntrynum(const_cast< char* >(DecodeARAMFile(filename))) != -1;
}
#endif

void CDvdFile::internalCallback(s32 res, DVDFileInfo* info) {
  if (res != DVD_STATE_CANCELED) {
    DCInvalidateRange(info->cb.addr, info->cb.length);
    lbl_80419B9D = true;
  }
}

void CDvdFile::CalcFileOffset(int offset, ESeekOrigin origin) {
  switch (origin) {
  case kSO_Set:
    mOffset = offset;
    break;
  case kSO_Cur:
    mOffset += offset;
    break;
  case kSO_End:
    mOffset = offset + mSize;
    break;
  }
}

void CDvdFile::UpdateFilePos(int pos) {
  mOffset += (pos + 31) & ~31;
  int filesize = GetFileSize();
  if (mOffset > filesize) {
    mOffset = filesize;
  }
}
