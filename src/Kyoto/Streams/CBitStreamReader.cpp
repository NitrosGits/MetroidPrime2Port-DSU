#include "Kyoto/Streams/CBitStreamReader.hpp"

#include "Kyoto/Alloc/CMemory.hpp"
#include "Kyoto/Streams/CInputStream.hpp"
#include "Kyoto/Streams/StreamSupport.hpp"

CBitStreamReader::CBitStreamReader(CInputStream& stream)
: mStream(stream), mBitWord(0), mBitOffset(0) {}

CBitStreamReader::~CBitStreamReader() {}

uint CBitStreamReader::ReadBits(uint bitCount) {
  if (mBitOffset >= bitCount) {
    uint mask = 0xffffffff;
    uint bwShift = 32 - bitCount;
    if (bitCount != 0x20) {
      mask = (1 << bitCount) - 1;
    }
#ifdef TARGET_PC
    uint ret = bwShift >= 32 ? 0 : mask & (mBitWord >> bwShift);

    mBitOffset -= bitCount;
    mBitWord = bitCount >= 32 ? 0 : mBitWord << bitCount;
#else
    uint ret = mask & (mBitWord >> bwShift);

    mBitOffset -= bitCount;
    mBitWord <<= bitCount;
#endif
    return ret;
  }

  uint shiftAmt = bitCount - mBitOffset;

  uint ret = 0;
  {
    uint mask = 0xffffffff;
    uint bwShift = 32 - mBitOffset;
    if (mBitOffset != 0x20) {
      mask = (1 << mBitOffset) - 1;
    }
#ifdef TARGET_PC
    // Shifts by 32 are undefined on the host (PPC gives 0): the result is 0 there.
    ret = (bwShift >= 32 || shiftAmt >= 32) ? 0 : (mask & (mBitWord >> bwShift)) << shiftAmt;
#else
    ret = (mask & (mBitWord >> bwShift)) << shiftAmt;
#endif
  }

  uint len = min_containing_bytes(shiftAmt);
  mBitOffset = 0;
#ifdef TARGET_PC
  // The word is big-endian on the stream (first byte = top bits); the host register is not.
  uchar bytes[4] = {0, 0, 0, 0};
  mStream.Get(bytes, len);
  mBitWord = (uint(bytes[0]) << 24) | (uint(bytes[1]) << 16) | (uint(bytes[2]) << 8) | bytes[3];
#else
  mStream.Get(&mBitWord, len);
#endif

  {
    uint mask = 0xffffffff;
    uint bwShift = 32 - shiftAmt;
    if (shiftAmt != 0x20) {
      mask = (1 << shiftAmt) - 1;
    }
#ifdef TARGET_PC
    ret |= (bwShift >= 32 ? 0 : ((mask & (mBitWord >> bwShift)) << mBitOffset));
#else
    ret |= ((mask & (mBitWord >> bwShift)) << mBitOffset);
#endif
  }

  mBitOffset = len * 8;
  mBitOffset -= shiftAmt;
#ifdef TARGET_PC
  mBitWord = shiftAmt >= 32 ? 0 : mBitWord << shiftAmt;
#else
  mBitWord <<= shiftAmt;
#endif
  return ret;
}

void CBitStreamReader::Flush() { mBitOffset = 0; }

CInputStream& CBitStreamReader::GetInputStream() {
  Flush();
  return mStream;
}
