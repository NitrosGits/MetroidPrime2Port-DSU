#include "Kyoto/Text/CStringTable.hpp"

#include "Kyoto/Alloc/CMemory.hpp"
#include "Kyoto/CDvdRequest.hpp"
#include "Kyoto/CFactoryMgr.hpp"
#include "Kyoto/Streams/CInputStream.hpp"

#include "rstl/algorithm.hpp"
#include "rstl/reserved_vector.hpp"

#include <string.h>

#ifdef TARGET_PC
#include <vector>

static uint ReadBE32(const uchar* p) {
  return uint(p[0]) << 24 | uint(p[1]) << 16 | uint(p[2]) << 8 | uint(p[3]);
}
#endif

static const wchar_t skInvalidString[] = L"Invalid";
static const FourCC skLanguages[] = {'ENGL', 'GERM', 'FREN', 'SPAN', 'ITAL', 'DUTC', 'JAPN'};
static FourCC mCurrentLanguage = skLanguages[0];

struct SLanguageInfo {
  FourCC mLanguage;
  uint mOffset;
  uint mSize;
};

struct SStringNameLess {
  template < typename T >
  bool operator()(const T& a, const T& b) const {
    return strcmp(a.mName, b.mName) < 0;
  }
};

void CStringTable::SetLanguage(int language) { mCurrentLanguage = skLanguages[language]; }

CStringTable::CStringTable(CInputStream& in)
: mStringCount(0)
, mNameCount(0)
, mData(nullptr)
, mNames(nullptr)
, mStrings(nullptr)
, mReloadData(nullptr) {
  Load(in);
}

CStringTable::~CStringTable() {}

void CStringTable::Load(CInputStream& in) {
  in.ReadInt32();
  uint version = in.ReadInt32();
  int langCount = in.Get(TType< int >());
  mStringCount = in.Get(TType< int >());

  rstl::reserved_vector< SLanguageInfo, 7 > languages;
  for (int i = 0; i < langCount; ++i) {
    FourCC language = in.Get(TType< FourCC >());
    uint offset = in.Get(TType< uint >());
    uint size = version != 0 ? in.Get(TType< uint >()) : 0;
    SLanguageInfo info = {language, offset, size};
    languages.push_back(info);
  }

  uint offset = languages[0].mOffset;
  uint size = languages[0].mSize;
  for (int i = 0; i < langCount; ++i) {
    if (languages[i].mLanguage == mCurrentLanguage) {
      offset = languages[i].mOffset;
      size = languages[i].mSize;
      break;
    }
  }

#ifdef TARGET_PC
  // Port: the disc holds 32-bit big-endian offsets and UTF-16BE text; a host has 64-bit pointers
  // and a 4-byte wchar_t. Read the raw names and language section, then lay mData out as host
  // structures: [SStringName x names][raw names blob][const wchar_t* x strings][wchar_t text].
  std::vector< uchar > rawNames;
  if (version != 0) {
    mNameCount = in.Get(TType< int >());
    uint namesSize = in.Get(TType< uint >());
    rawNames.resize(namesSize);
    in.ReadBytes(rawNames.data(), namesSize);
  }
  for (uint i = 0; i < offset; ++i) {
    in.ReadInt8();
  }
  if (version == 0) {
    size = in.Get(TType< uint >());
  }
  std::vector< uchar > section(size);
  in.ReadBytes(section.data(), size);

  if (mNameCount < 0 || size_t(mNameCount) * 8 > rawNames.size()) {
    mNameCount = 0;
  }
  if (mStringCount < 0 || size_t(mStringCount) * 4 > section.size()) {
    mStringCount = 0;
  }
  size_t textUnits = 0;
  for (int i = 0; i < mStringCount; ++i) {
    for (size_t p = ReadBE32(&section[i * 4]); p + 1 < section.size(); p += 2, ++textUnits) {
      if (section[p] == 0 && section[p + 1] == 0) {
        break;
      }
    }
    ++textUnits;
  }

  const size_t namesBytes = size_t(mNameCount) * sizeof(SStringName);
  const size_t ptrsAt = (namesBytes + rawNames.size() + 7) & ~size_t(7);
  const size_t textAt = ptrsAt + size_t(mStringCount) * sizeof(const wchar_t*);
  mData = rs_new uchar[textAt + textUnits * sizeof(wchar_t)];
  uchar* base = mData.get();

  mNames = reinterpret_cast< SStringName* >(base);
  char* blob = reinterpret_cast< char* >(base + namesBytes);
  memcpy(blob, rawNames.data(), rawNames.size());
  for (int i = 0; i < mNameCount; ++i) {
    const uint nameAt = ReadBE32(&rawNames[i * 8]);
    mNames[i].mName = nameAt < rawNames.size() ? blob + nameAt : "";
    mNames[i].mIndex = int(ReadBE32(&rawNames[i * 8 + 4]));
  }

  mStrings = reinterpret_cast< const wchar_t** >(base + ptrsAt);
  wchar_t* text = reinterpret_cast< wchar_t* >(base + textAt);
  for (int i = 0; i < mStringCount; ++i) {
    mStrings[i] = text;
    for (size_t p = ReadBE32(&section[i * 4]); p + 1 < section.size(); p += 2) {
      const wchar_t unit = wchar_t(section[p] << 8 | section[p + 1]);
      if (unit == 0) {
        break;
      }
      *text++ = unit;
    }
    *text++ = 0;
  }
#else
  if (version != 0) {
    mNameCount = in.Get(TType< int >());
    uint namesSize = in.Get(TType< uint >());
    mData = rs_new uchar[namesSize + size];
    mNames = reinterpret_cast< SStringName* >(mData.get());
    mStrings = reinterpret_cast< const wchar_t** >(mData.get() + namesSize);
    in.ReadBytes(mNames, namesSize);
  }

  for (uint i = 0; i < offset; ++i) {
    in.ReadInt8();
  }

  if (version == 0) {
    size = in.Get(TType< uint >());
    mData = rs_new uchar[size];
    mStrings = reinterpret_cast< const wchar_t** >(mData.get());
  }
  in.ReadBytes(mStrings, size);

  if (version != 0) {
    SStringName* entry = mNames;
    for (int i = 0; i < mNameCount; ++i, ++entry) {
      entry->mName += reinterpret_cast< uint >(mNames);
    }
  }

  uint* entry = reinterpret_cast< uint* >(mStrings);
  for (int i = 0; i < mStringCount; ++i, ++entry) {
    *entry += reinterpret_cast< uint >(mStrings);
  }
#endif
}

const wchar_t* CStringTable::GetString(int idx) const {
  if (idx < 0 || idx >= mStringCount) {
    return skInvalidString;
  }
  return mStrings[idx];
}

int CStringTable::GetStringIndex(const char* name) const {
  SStringName* begin = mNames;
  if (mNameCount > 0 && *name != '\0') {
    SStringName* end = begin + mNameCount;
    SStringName key = {name};
    SStringNameLess cmp;
    SStringName* it = rstl::lower_bound(begin, end, key, cmp);
    if (it != end && strcmp(it->mName, name) == 0) {
      return it->mIndex;
    }
  }
  return -1;
}

const wchar_t* CStringTable::GetString(const char* name) const {
  return GetString(GetStringIndex(name));
}

CStringTable::SReloadData::~SReloadData() {}

CFactoryFnReturn FStringTableFactory(const SObjectTag& tag, CInputStream& in,
                                     const CVParamTransfer& xfer) {
  return rs_new CStringTable(in);
}
