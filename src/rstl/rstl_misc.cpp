#include "rstl/rmemory_allocator.hpp"

#ifdef TARGET_PC
#include "Kyoto/Alloc/CMemory.hpp"
#endif

void* rstl::rmemory_allocator::allocate(int size) {
#ifdef TARGET_PC
  // Port: route through CMemory so CMemory::Free can release it on the host.
  if (size == 0) {
    return nullptr;
  }
  return CMemory::Alloc(static_cast< size_t >(size), IAllocator::kHI_None, IAllocator::kSC_Unk1,
                        IAllocator::kTP_Array, CCallStack(-1, "??(??)", nullptr));
#else
  return size == 0 ? nullptr : rs_new uchar[size];
#endif
}
