// Save states, room trailer format (MP1's port_savestate_format.cpp, adapted to MP2). See
// platform/include/port_savestate.h.
//
// v4 (what this writes) is v3 with a spawn-layer byte (and u64 mask) after each area's known byte.
// v2 (what old .mp2ss files carry; no version word): u32 area count, (area, assetId) pairs, u32
// object count, per object u32 editorId, u32 active, u32 block count, blocks of (u32 tag, u32 length,
// bytes). v3 starts with kV3Magic, which no v2 area count can equal (v2 counts stop at kMaxCount),
// and is compact: per area a fingerprint of its script data, objects sorted by editor id and stored
// as varint deltas, a flags byte (0x80 active | block count) and blocks of (u8 tag, u16 length).

#include "port_savestate.h"

#include "MetroidPrime/CGameArea.hpp"

#include <algorithm>
#include <cstring>

namespace PortSaveState {
namespace {

constexpr uint32_t kV4Magic = 0x34534d52; // "RMS4": v3 plus per-area spawn layers
constexpr uint32_t kMaxCount = 1u << 20;
constexpr size_t kMaxBlocks = 64; // a v2 limit; v3's flags byte holds 7 bits
constexpr size_t kMaxBlock = 0xffff;

struct Out {
  std::string bytes;
  void U8(uint8_t v) { bytes.push_back(static_cast< char >(v)); }
  void U16(uint16_t v) {
    U8(static_cast< uint8_t >(v));
    U8(static_cast< uint8_t >(v >> 8));
  }
  void U32(uint32_t v) {
    for (int i = 0; i < 4; ++i) {
      U8(static_cast< uint8_t >(v >> (8 * i)));
    }
  }
  void U64(uint64_t v) {
    U32(static_cast< uint32_t >(v));
    U32(static_cast< uint32_t >(v >> 32));
  }
  void Varint(uint32_t v) {
    while (v >= 0x80) {
      U8(static_cast< uint8_t >(v | 0x80));
      v >>= 7;
    }
    U8(static_cast< uint8_t >(v));
  }
};

struct In {
  const std::string& in;
  size_t at = 0;
  bool ok = true;
  explicit In(const std::string& s) : in(s) {}
  bool Need(size_t n) {
    if (!ok || n > in.size() - at) {
      ok = false;
    }
    return ok;
  }
  uint8_t U8() {
    if (!Need(1)) {
      return 0;
    }
    return static_cast< uint8_t >(in[at++]);
  }
  uint16_t U16() {
    const uint16_t lo = U8();
    return static_cast< uint16_t >(lo | (U8() << 8));
  }
  uint32_t U32() {
    uint32_t v = 0;
    if (!Need(4)) {
      return 0;
    }
    for (int i = 0; i < 4; ++i) {
      v |= static_cast< uint32_t >(static_cast< uint8_t >(in[at + i])) << (8 * i);
    }
    at += 4;
    return v;
  }
  uint64_t U64() {
    const uint64_t lo = U32();
    return lo | (static_cast< uint64_t >(U32()) << 32);
  }
  // A varint of at most five bytes that fits 32 bits.
  uint32_t Varint() {
    uint64_t v = 0;
    for (int shift = 0; shift < 35; shift += 7) {
      const uint8_t b = U8();
      if (!ok) {
        return 0;
      }
      v |= static_cast< uint64_t >(b & 0x7f) << shift;
      if ((b & 0x80) == 0) {
        if (v > 0xffffffffull) {
          ok = false;
          return 0;
        }
        return static_cast< uint32_t >(v);
      }
    }
    ok = false;
    return 0;
  }
  std::string Bytes(size_t len) {
    if (!Need(len)) {
      return std::string();
    }
    std::string s = in.substr(at, len);
    at += len;
    return s;
  }
};

bool DecodeV2(In& r, RoomState& room) {
  const uint32_t areas = r.U32();
  if (!r.ok || areas > kMaxCount) {
    return false;
  }
  for (uint32_t i = 0; i < areas && r.ok; ++i) {
    RoomArea a;
    a.area = static_cast< int32_t >(r.U32());
    a.assetId = r.U32();
    room.areas.push_back(a);
  }
  const uint32_t objects = r.U32();
  if (!r.ok || objects > kMaxCount) {
    return false;
  }
  room.objects.reserve(std::min< uint32_t >(objects, 4096));
  for (uint32_t i = 0; i < objects && r.ok; ++i) {
    RoomObject obj;
    obj.editorId = r.U32();
    obj.active = r.U32() != 0;
    const uint32_t blocks = r.U32();
    if (blocks > kMaxBlocks) {
      return false;
    }
    for (uint32_t b = 0; b < blocks && r.ok; ++b) {
      RoomBlock block;
      block.tag = r.U32();
      block.data = r.Bytes(r.U32());
      obj.blocks.push_back(std::move(block));
    }
    room.objects.push_back(std::move(obj));
  }
  return r.ok;
}

bool DecodeV3(In& r, RoomState& room, bool v4) {
  const uint32_t areas = r.U32();
  if (!r.ok || areas > kMaxCount) {
    return false;
  }
  int32_t last = -1;
  for (uint32_t i = 0; i < areas && r.ok; ++i) {
    RoomArea a;
    a.area = static_cast< int32_t >(r.U32());
    a.assetId = r.U32();
    a.fingerprint = r.U64();
    const uint8_t known = r.U8();
    // Areas rise and are non-negative; anything else is a damaged or hostile file.
    if (!r.ok || a.area <= last || known > 1) {
      return false;
    }
    a.known = known == 1;
    if (v4) {
      const uint8_t spawn = r.U8();
      if (spawn > 1) {
        return false;
      }
      a.hasSpawn = spawn == 1;
      if (a.hasSpawn) {
        a.spawnLayers = r.U64();
      }
    }
    last = a.area;
    room.areas.push_back(a);
  }
  const uint32_t objects = r.U32();
  if (!r.ok || objects > kMaxCount) {
    return false;
  }
  room.objects.reserve(std::min< uint32_t >(objects, 4096));
  uint32_t id = 0;
  for (uint32_t i = 0; i < objects && r.ok; ++i) {
    RoomObject obj;
    const uint32_t delta = r.Varint();
    if (static_cast< uint64_t >(id) + delta > 0xffffffffull) {
      return false;
    }
    id += delta;
    obj.editorId = id;
    const uint8_t flags = r.U8();
    obj.active = (flags & 0x80) != 0;
    for (int b = 0; b < (flags & 0x7f) && r.ok; ++b) {
      RoomBlock block;
      block.tag = r.U8();
      block.data = r.Bytes(r.U16());
      obj.blocks.push_back(std::move(block));
    }
    room.objects.push_back(std::move(obj));
  }
  // Nothing may trail the last object: a blob cut short or padded is not this trailer.
  return r.ok && r.at == r.in.size();
}

} // namespace

uint64_t Fnv1a64(const void* data, size_t size, uint64_t seed) {
  const uint8_t* p = static_cast< const uint8_t* >(data);
  uint64_t h = seed;
  for (size_t i = 0; i < size; ++i) {
    h = (h ^ p[i]) * 0x100000001b3ull;
  }
  return h;
}

// Over the layer count and each layer's script size plus the generated script size. The script
// buffers themselves are freed once a layer has loaded (and half filled while it loads), so only
// the sizes are stable; a patched area changes them.
uint64_t AreaFingerprint(const CGameArea& area) {
  uint64_t h = 0xcbf29ce484222325ull;
  const int layers = area.PortLayerCount();
  const uint32_t count = static_cast< uint32_t >(layers);
  h = Fnv1a64(&count, sizeof count, h);
  for (int i = 0; i < layers; ++i) {
    const uint32_t head[2] = {static_cast< uint32_t >(i),
                              static_cast< uint32_t >(area.GetLayerScriptSize(TLayerId(i)))};
    h = Fnv1a64(head, sizeof head, h);
  }
  const uint32_t gen = static_cast< uint32_t >(area.GetGeneratedScriptBuffer().second);
  return Fnv1a64(&gen, sizeof gen, h);
}

const char* RoomAreaSkipReason(const RoomArea& saved, uint64_t liveFingerprint) {
  if (saved.known && saved.fingerprint != liveFingerprint) {
    return "has other scripts than when the state was made";
  }
  return nullptr;
}

void EncodeRoomState(const RoomState& room, std::string& out) {
  std::vector< const RoomObject* > sorted;
  sorted.reserve(room.objects.size());
  bool fits = true;
  for (const RoomObject& obj : room.objects) {
    sorted.push_back(&obj);
    fits = fits && obj.blocks.size() <= 0x7f;
    for (const RoomBlock& b : obj.blocks) {
      fits = fits && b.tag <= 0xff && b.data.size() <= kMaxBlock;
    }
  }
  // A block that does not fit its length field would be cut silently: write no objects instead.
  const bool keep = fits && room.objects.size() <= kMaxCount;
  std::stable_sort(sorted.begin(), sorted.end(), [](const RoomObject* a, const RoomObject* b) {
    return a->editorId < b->editorId;
  });
  Out w;
  w.U32(kV4Magic);
  w.U32(static_cast< uint32_t >(room.areas.size()));
  for (const RoomArea& a : room.areas) {
    w.U32(static_cast< uint32_t >(a.area));
    w.U32(a.assetId);
    w.U64(a.fingerprint);
    w.U8(a.known ? 1 : 0);
    w.U8(a.hasSpawn ? 1 : 0);
    if (a.hasSpawn) {
      w.U64(a.spawnLayers);
    }
  }
  w.U32(keep ? static_cast< uint32_t >(sorted.size()) : 0);
  uint32_t id = 0;
  for (size_t i = 0; keep && i < sorted.size(); ++i) {
    const RoomObject& obj = *sorted[i];
    w.Varint(obj.editorId - id);
    id = obj.editorId;
    w.U8(static_cast< uint8_t >((obj.active ? 0x80 : 0) | obj.blocks.size()));
    for (const RoomBlock& b : obj.blocks) {
      w.U8(static_cast< uint8_t >(b.tag));
      w.U16(static_cast< uint16_t >(b.data.size()));
      w.bytes.append(b.data);
    }
  }
  out = std::move(w.bytes);
}

bool DecodeRoomState(const std::string& in, RoomState& room) {
  room = RoomState();
  In r(in);
  // v3 leads with its magic; a v2 trailer leads with an area count, always below kMaxCount.
  bool ok;
  if (in.size() >= 4 && (std::memcmp(in.data(), "RMS3", 4) == 0 || std::memcmp(in.data(), "RMS4", 4) == 0)) {
    const bool v4 = in[3] == '4';
    r.U32();
    ok = DecodeV3(r, room, v4);
  } else {
    ok = DecodeV2(r, room);
  }
  if (!ok) {
    room = RoomState();
  }
  return ok;
}

} // namespace PortSaveState
