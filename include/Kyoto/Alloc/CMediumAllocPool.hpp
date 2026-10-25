#ifndef _CMEDIUMALLOCPOOL
#define _CMEDIUMALLOCPOOL

#include "Kyoto/Alloc/AllocatorCommon.hpp"
#include <rstl/auto_ptr.hpp>
#include <rstl/list.hpp>

struct SMediumAllocPuddle {
  SMediumAllocPuddle(const uint numBlocks, void* data, const bool canErase);
  ~SMediumAllocPuddle();
  void* FindFree(uint blockCount);
  void* FindFreeEntry(uint blockCount);
  void Free(const void* ptr);

  const uint GetNumBlocks() const { return mNumBlocks; }
  const uint GetNumAllocs() const { return mNumAllocs; }
  const uint GetNumEntries() const { return mNumEntries; }
  const bool CanErase() const { return mCanErase; }
#ifdef TARGET_PC
  // size_t, not uint: a 64-bit difference truncated to 32 bits can land a pointer from elsewhere
  // inside the puddle.
  const size_t GetPtrOffset(const void* ptr) const {
    return size_t(static_cast< const uchar* >(ptr) - mMainData.get());
  }
#else
  const uint GetPtrOffset(const void* ptr) const { return (uchar*)ptr - mMainData.get(); }
#endif
  static ushort GetBlockOffset(const void* ptrA, const void* ptrB);
  static void InitBookKeeping(uchar* bookKeepingPtr, const ushort blockCount);

private:
  rstl::auto_ptr< uchar > mMainData;
  uchar* mBookKeeping;
  uchar* mCachedBookKeepingAddr;
  uint mUnused;
  uint mNumBlocks;
  uint mNumAllocs;
  uint mNumEntries;
  bool mCanErase : 1;
};

class CMediumAllocPool {
public:
  rstl::list< SMediumAllocPuddle > mList;
  rstl::list< SMediumAllocPuddle >::iterator mLastNodePrev;
  CMediumAllocPool();
  void* Alloc(uint size);
  bool HasPuddles() const;
  void AddPuddle(const uint, void*, const bool);
  void ClearPuddles();

  int Free(const void* ptr);

  uint GetTotalEntries();
  uint GetNumBlocksAvailable();
  uint GetNumAllocs();

  static uint GetAllocMemoryRequired(uint numBlocks) { return numBlocks * 32; }
  static uint GetBookKeepingMemoryRequired(uint numBlocks) { return numBlocks; }

  static CMediumAllocPool* gMediumAllocPtr;
};

#endif // _CMEDIUMALLOCPOOL
