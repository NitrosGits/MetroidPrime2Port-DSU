#include "port_rando_seed.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>

namespace PortRandoSeed {
namespace {

struct ParseError {
  size_t offset;
  std::string reason;
};

struct Value {
  enum Type { Null, Bool, Int, Float, String, Array, Object } type = Null;
  bool boolean = false;
  long long integer = 0;
  std::string string;
  std::vector<Value> array;
  std::vector<std::pair<std::string, Value>> object;

  const Value* Find(const char* key) const {
    if (type != Object)
      return nullptr;
    for (const auto& member : object) {
      if (member.first == key)
        return &member.second;
    }
    return nullptr;
  }
};

// A JSON reader into a Value tree (same string-escape rules as the native seed parser in
// port_randomizer.cpp, which cannot read arrays).
class Reader {
  const std::string& mText;
  size_t mPos = 0;

  [[noreturn]] void Fail(const char* reason) const { throw ParseError{mPos, reason}; }

  void SkipWhitespace() {
    while (mPos < mText.size() && (mText[mPos] == ' ' || mText[mPos] == '\t' ||
                                   mText[mPos] == '\r' || mText[mPos] == '\n'))
      ++mPos;
  }

  bool Consume(char c) {
    SkipWhitespace();
    if (mPos < mText.size() && mText[mPos] == c) {
      ++mPos;
      return true;
    }
    return false;
  }

  void Expect(char c) {
    if (!Consume(c))
      Fail("unexpected token");
  }

  static void AppendUtf8(std::string& out, unsigned int cp) {
    if (cp <= 0x7f) {
      out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7ff) {
      out.push_back(static_cast<char>(0xc0 | (cp >> 6)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else if (cp <= 0xffff) {
      out.push_back(static_cast<char>(0xe0 | (cp >> 12)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else {
      out.push_back(static_cast<char>(0xf0 | (cp >> 18)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    }
  }

  unsigned int ParseHex4() {
    if (mText.size() - mPos < 4)
      Fail("incomplete unicode escape");
    unsigned int value = 0;
    for (int i = 0; i < 4; ++i) {
      const char c = mText[mPos++];
      value <<= 4;
      if (c >= '0' && c <= '9')
        value |= static_cast<unsigned int>(c - '0');
      else if (c >= 'a' && c <= 'f')
        value |= static_cast<unsigned int>(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F')
        value |= static_cast<unsigned int>(c - 'A' + 10);
      else
        Fail("invalid unicode escape");
    }
    return value;
  }

  std::string ParseString() {
    Expect('"');
    std::string result;
    while (mPos < mText.size()) {
      const unsigned char c = static_cast<unsigned char>(mText[mPos++]);
      if (c == '"')
        return result;
      if (c < 0x20)
        Fail("unescaped control character in string");
      if (c != '\\') {
        result.push_back(static_cast<char>(c));
        continue;
      }
      if (mPos >= mText.size())
        Fail("incomplete string escape");
      switch (mText[mPos++]) {
      case '"': result.push_back('"'); break;
      case '\\': result.push_back('\\'); break;
      case '/': result.push_back('/'); break;
      case 'b': result.push_back('\b'); break;
      case 'f': result.push_back('\f'); break;
      case 'n': result.push_back('\n'); break;
      case 'r': result.push_back('\r'); break;
      case 't': result.push_back('\t'); break;
      case 'u': {
        unsigned int cp = ParseHex4();
        if (cp >= 0xd800 && cp <= 0xdbff) {
          if (mText.size() - mPos < 6 || mText[mPos] != '\\' || mText[mPos + 1] != 'u')
            Fail("missing low surrogate");
          mPos += 2;
          const unsigned int low = ParseHex4();
          if (low < 0xdc00 || low > 0xdfff)
            Fail("invalid low surrogate");
          cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
        } else if (cp >= 0xdc00 && cp <= 0xdfff) {
          Fail("unexpected low surrogate");
        }
        AppendUtf8(result, cp);
        break;
      }
      default: Fail("invalid string escape");
      }
    }
    Fail("unterminated string");
  }

  static bool IsDigit(char c) { return c >= '0' && c <= '9'; }

  void ParseNumber(Value& v) {
    const size_t start = mPos;
    if (mText[mPos] == '-')
      ++mPos;
    if (mPos >= mText.size() || !IsDigit(mText[mPos]))
      Fail("expected number");
    if (mText[mPos] == '0') {
      ++mPos;
      if (mPos < mText.size() && IsDigit(mText[mPos]))
        Fail("leading zero in number");
    } else {
      while (mPos < mText.size() && IsDigit(mText[mPos]))
        ++mPos;
    }
    bool isFloat = false;
    if (mPos < mText.size() && mText[mPos] == '.') {
      isFloat = true;
      ++mPos;
      if (mPos >= mText.size() || !IsDigit(mText[mPos]))
        Fail("expected digits after decimal point");
      while (mPos < mText.size() && IsDigit(mText[mPos]))
        ++mPos;
    }
    if (mPos < mText.size() && (mText[mPos] == 'e' || mText[mPos] == 'E')) {
      isFloat = true;
      ++mPos;
      if (mPos < mText.size() && (mText[mPos] == '+' || mText[mPos] == '-'))
        ++mPos;
      if (mPos >= mText.size() || !IsDigit(mText[mPos]))
        Fail("expected exponent digits");
      while (mPos < mText.size() && IsDigit(mText[mPos]))
        ++mPos;
    }
    if (isFloat) {
      v.type = Value::Float;
      return;
    }
    const std::string digits = mText.substr(start, mPos - start);
    errno = 0;
    v.integer = std::strtoll(digits.c_str(), nullptr, 10);
    if (errno != 0)
      Fail("integer out of range");
    v.type = Value::Int;
  }

  bool ConsumeLiteral(const char* literal) {
    const size_t length = std::strlen(literal);
    if (mText.compare(mPos, length, literal) != 0)
      return false;
    mPos += length;
    return true;
  }

  void ParseValue(Value& v, int depth) {
    if (depth > 128)
      Fail("nesting limit exceeded");
    SkipWhitespace();
    if (mPos >= mText.size())
      Fail("expected value");
    const char c = mText[mPos];
    if (c == '"') {
      v.type = Value::String;
      v.string = ParseString();
    } else if (c == '{') {
      ++mPos;
      v.type = Value::Object;
      if (Consume('}'))
        return;
      for (;;) {
        std::string key = ParseString();
        Expect(':');
        v.object.emplace_back(std::move(key), Value());
        ParseValue(v.object.back().second, depth + 1);
        if (Consume('}'))
          return;
        Expect(',');
      }
    } else if (c == '[') {
      ++mPos;
      v.type = Value::Array;
      if (Consume(']'))
        return;
      for (;;) {
        v.array.emplace_back();
        ParseValue(v.array.back(), depth + 1);
        if (Consume(']'))
          return;
        Expect(',');
      }
    } else if (c == '-' || IsDigit(c)) {
      ParseNumber(v);
    } else if (ConsumeLiteral("true")) {
      v.type = Value::Bool;
      v.boolean = true;
    } else if (ConsumeLiteral("false")) {
      v.type = Value::Bool;
    } else if (ConsumeLiteral("null")) {
      v.type = Value::Null;
    } else {
      Fail("unexpected token");
    }
  }

public:
  explicit Reader(const std::string& text) : mText(text) {}

  void Parse(Value& root) {
    ParseValue(root, 0);
    SkipWhitespace();
    if (mPos != mText.size())
      Fail("trailing data");
  }
};

// ---- Value -> Seed ----

[[noreturn]] void Bad(const std::string& reason) { throw ParseError{0, reason}; }

int AsInt(const Value& v, const char* what) {
  if (v.type != Value::Int || v.integer < -2147483648LL || v.integer > 2147483647LL)
    Bad(std::string(what) + " must be an integer");
  return static_cast<int>(v.integer);
}

uint32_t AsU32(const Value& v, const char* what) {
  if (v.type != Value::Int || v.integer < 0 || v.integer > 0xFFFFFFFFLL)
    Bad(std::string(what) + " must be an unsigned 32-bit integer");
  return static_cast<uint32_t>(v.integer);
}

const std::string& AsString(const Value& v, const char* what) {
  if (v.type != Value::String)
    Bad(std::string(what) + " must be a string");
  return v.string;
}

const std::vector<Value>& AsArray(const Value& v, const char* what) {
  if (v.type != Value::Array)
    Bad(std::string(what) + " must be an array");
  return v.array;
}

const Value& Require(const Value& obj, const char* key, const char* what) {
  if (obj.type != Value::Object)
    Bad(std::string(what) + " must be an object");
  const Value* found = obj.Find(key);
  if (found == nullptr)
    Bad(std::string(what) + " is missing \"" + key + "\"");
  return *found;
}

std::vector<ResourceGrant> ReadResources(const Value& v, const char* what, bool dropMultiworld) {
  std::vector<ResourceGrant> out;
  for (const Value& entry : AsArray(v, what)) {
    ResourceGrant grant;
    grant.item = AsInt(Require(entry, "index", what), "resource index");
    grant.amount = AsInt(Require(entry, "amount", what), "resource amount");
    if (dropMultiworld && grant.item == kMultiworldItem)
      continue;
    out.push_back(grant);
  }
  return out;
}

void ReadLocation(const Value& v, const char* what, uint32_t& world, uint32_t& area) {
  world = AsU32(Require(v, "world_asset_id", what), "world_asset_id");
  area = AsU32(Require(v, "area_asset_id", what), "area_asset_id");
}

Pickup ReadPickup(const Value& v) {
  Pickup p;
  p.pickupIndex = AsInt(Require(v, "pickup_index", "pickup"), "pickup_index");
  if (const Value* r = v.Find("resources"))
    p.resources = ReadResources(*r, "resources", true);
  if (const Value* c = v.Find("conditional_resources")) {
    for (const Value& entry : AsArray(*c, "conditional_resources")) {
      ConditionalResource cond;
      cond.item = AsInt(Require(entry, "item", "conditional resource"), "conditional item");
      cond.resources =
          ReadResources(Require(entry, "resources", "conditional resource"), "resources", true);
      p.conditionalResources.push_back(std::move(cond));
    }
  }
  if (const Value* c = v.Find("convert")) {
    for (const Value& entry : AsArray(*c, "convert")) {
      Conversion conv;
      conv.fromItem = AsInt(Require(entry, "from_item", "convert"), "from_item");
      conv.toItem = AsInt(Require(entry, "to_item", "convert"), "to_item");
      if (const Value* b = entry.Find("clear_source"))
        conv.clearSource = b->type == Value::Bool && b->boolean;
      if (const Value* b = entry.Find("overwrite_target"))
        conv.overwriteTarget = b->type == Value::Bool && b->boolean;
      p.conversions.push_back(conv);
    }
  }
  if (const Value* h = v.Find("hud_text")) {
    for (const Value& line : AsArray(*h, "hud_text"))
      p.hudText.push_back(AsString(line, "hud_text entry"));
  }
  if (const Value* s = v.Find("scan"))
    p.scan = AsString(*s, "scan");
  if (const Value* m = v.Find("model")) {
    if (m->type == Value::Object) {
      if (const Value* g = m->Find("game"))
        p.modelGame = AsString(*g, "model game");
      if (const Value* n = m->Find("name"))
        p.modelName = AsString(*n, "model name");
    }
  }
  if (const Value* i = v.Find("model_index"))
    p.modelIndex = AsInt(*i, "model_index");
  if (const Value* i = v.Find("sound_index"))
    p.soundIndex = AsInt(*i, "sound_index");
  if (const Value* i = v.Find("jingle_index"))
    p.jingleIndex = AsInt(*i, "jingle_index");
  return p;
}

void ReadSeed(const Value& root, Seed& out) {
  if (root.type != Value::Object)
    Bad("patch data must be a JSON object");
  out = Seed();
  out.name = "Randovania";
  if (const Value* banner = root.Find("banner_name")) {
    if (banner->type == Value::String && !banner->string.empty())
      out.name = banner->string;
  }
  for (const Value& entry : AsArray(Require(root, "pickups", "patch data"), "pickups"))
    out.pickups.push_back(ReadPickup(entry));

  if (const Value* spawn = root.Find("spawn_point")) {
    if (spawn->type == Value::Object) {
      out.spawn.present = true;
      ReadLocation(Require(*spawn, "location", "spawn_point"), "spawn_point location",
                   out.spawn.worldAssetId, out.spawn.areaAssetId);
      if (const Value* a = spawn->Find("amount"))
        out.spawn.amount = ReadResources(*a, "spawn_point amount", false);
      if (const Value* c = spawn->Find("capacity"))
        out.spawn.capacity = ReadResources(*c, "spawn_point capacity", false);
    }
  }
  if (const Value* elevators = root.Find("elevators")) {
    for (const Value& entry : AsArray(*elevators, "elevators")) {
      Elevator e;
      e.instanceId = AsU32(Require(entry, "instance_id", "elevator"), "instance_id");
      ReadLocation(Require(entry, "origin_location", "elevator"), "origin_location",
                   e.originWorldAssetId, e.originAreaAssetId);
      ReadLocation(Require(entry, "target_location", "elevator"), "target_location",
                   e.targetWorldAssetId, e.targetAreaAssetId);
      if (const Value* n = entry.Find("room_name"))
        e.roomName = AsString(*n, "room_name");
      out.elevators.push_back(std::move(e));
    }
  }
  if (const Value* gates = root.Find("translator_gates")) {
    for (const Value& entry : AsArray(*gates, "translator_gates")) {
      TranslatorGate g;
      g.gateIndex = AsInt(Require(entry, "gate_index", "translator gate"), "gate_index");
      g.translatorIndex =
          AsInt(Require(entry, "translator_index", "translator gate"), "translator_index");
      out.translatorGates.push_back(g);
    }
  }
}

} // namespace

bool IsRandovania(const std::string& json) {
  try {
    Value root;
    Reader(json).Parse(root);
    return root.Find("pickups") != nullptr;
  } catch (const ParseError&) {
    return false;
  }
}

bool ParseRandovania(const std::string& json, Seed& out, std::string& error) {
  error.clear();
  try {
    Value root;
    Reader(json).Parse(root);
    ReadSeed(root, out);
    return true;
  } catch (const ParseError& e) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), " (byte offset %zu)", e.offset);
    error = e.reason + (e.offset != 0 ? buffer : "");
    return false;
  }
}

} // namespace PortRandoSeed
