// Suit colour customisation's image work and picks (port_suit_colours.h). The
// disc reading and the cache are in port_suit_colours_disc.cpp.

#include "port_suit_colours.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <random>
#include <utility>

namespace PortSuitColours {
namespace {

// The slot medians were measured on the USA disc over each suit's arm cannon and
// morph ball textures (hue clusters by circular k-means over the texels that
// pass the coloured test below; docs/MODS.md "Suit colours"). Dark Samus's copper
// armour and red glow share a hue, so they are one slot (the glow follows the
// pick, brighter); the Light Suit is silver and white with gold trim, so its one
// slot is the trim.
const Suit kSuits[kSuitCount] = {
    {"varia", "Varia Suit", 2, {{{38.9, 0.821, 0.902}, {5.0, 0.935, 0.675}, {}}},
     {"primary", "secondary", nullptr}, {"Primary", "Secondary", nullptr}, true, false},
    {"dark", "Dark Suit", 1, {{{14.5, 0.510, 0.392}, {}, {}}},
     {"primary", nullptr, nullptr}, {"Armour", nullptr, nullptr}, true, false},
    {"light", "Light Suit", 1, {{{41.1, 0.415, 0.290}, {}, {}}},
     {"accent", nullptr, nullptr}, {"Accent", nullptr, nullptr}, true, false},
};

// Tuned by eye on contact sheets of every suit.
constexpr double kGateFull = 25.0;     // degrees from the nearest median: full recolour below
constexpr double kGateZero = 35.0;     // and none from here
constexpr double kCompress = 0.35;     // how much of a texel's offset from its median it keeps
constexpr double kIslandShare = 0.70;  // an island takes one slot when this share of it has it
constexpr int kIslandMin = 40;         // smaller islands take their majority whatever it is
constexpr double kIslandHue = 60.0;    // but only texels this near that slot's hue
constexpr double kSeamValue = 0.18;    // darker texels part islands
constexpr double kScaleMax = 4.0;      // saturation and value scale at most this much

std::mutex sMutex;
std::array<Picks, kSuitCount> sPicks{};
std::array<std::string, kSuitCount> sServed{};

double Smooth(double x, double a, double b) {
  const double t = std::clamp((x - a) / (b - a), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}

double HueDistance(double a, double b) {
  const double d = std::fmod(std::fabs(a - b), 360.0);
  return std::min(d, 360.0 - d);
}

// -180..180.
double Wrap(double d) {
  double w = std::fmod(d + 180.0, 360.0);
  if (w < 0.0) {
    w += 360.0;
  }
  return w - 180.0;
}

Hsv ToHsv(double r, double g, double b) {
  const double max = std::max({r, g, b});
  const double min = std::min({r, g, b});
  const double range = max - min;
  Hsv out;
  out.v = max;
  out.s = max > 1e-6 ? range / max : 0.0;
  if (range > 1e-6) {
    if (max == r) {
      out.h = 60.0 * std::fmod((g - b) / range + 6.0, 6.0);
    } else if (max == g) {
      out.h = 60.0 * ((b - r) / range + 2.0);
    } else {
      out.h = 60.0 * ((r - g) / range + 4.0);
    }
  }
  return out;
}

// 0..255 floats.
void FromHsv(double h, double s, double v, double* rgb) {
  h = std::fmod(std::fmod(h, 360.0) + 360.0, 360.0) / 60.0;
  const int i = int(std::floor(h)) % 6;
  const double f = h - std::floor(h);
  const double p = v * (1.0 - s), q = v * (1.0 - s * f), t = v * (1.0 - s * (1.0 - f));
  double r = v, g = t, b = p;
  switch (i) {
  case 1: r = q, g = v, b = p; break;
  case 2: r = p, g = v, b = t; break;
  case 3: r = p, g = q, b = v; break;
  case 4: r = t, g = p, b = v; break;
  case 5: r = v, g = p, b = q; break;
  default: break;
  }
  rgb[0] = r * 255.0;
  rgb[1] = g * 255.0;
  rgb[2] = b * 255.0;
}

// The sums of `in` over the k x k window round each texel (zero outside).
std::vector<int> BoxSum(const std::vector<uint8_t>& in, int width, int height, int k) {
  const int r = k / 2;
  std::vector<int> table(size_t(width + 1) * size_t(height + 1), 0);
  for (int y = 0; y < height; ++y) {
    int row = 0;
    for (int x = 0; x < width; ++x) {
      row += in[size_t(y) * width + x];
      table[size_t(y + 1) * (width + 1) + (x + 1)] = table[size_t(y) * (width + 1) + (x + 1)] + row;
    }
  }
  std::vector<int> out(in.size());
  for (int y = 0; y < height; ++y) {
    const int y0 = std::max(y - r, 0), y1 = std::min(y + r + 1, height);
    for (int x = 0; x < width; ++x) {
      const int x0 = std::max(x - r, 0), x1 = std::min(x + r + 1, width);
      out[size_t(y) * width + x] = table[size_t(y1) * (width + 1) + x1] - table[size_t(y0) * (width + 1) + x1] -
                                   table[size_t(y1) * (width + 1) + x0] + table[size_t(y0) * (width + 1) + x0];
    }
  }
  return out;
}

// A texture as HSV, and which slot each texel belongs to.
struct Analysis {
  int width = 0, height = 0;
  std::vector<Hsv> hsv;
  std::vector<double> coloured;  // 0..1
  std::vector<double> nearest;   // degrees to the nearest slot median
  std::vector<int> slot;         // -1: not recoloured
  std::vector<uint8_t> hard;     // the slot came from a whole island
};

Analysis Analyse(const Image& image, int suit) {
  const Suit& info = kSuits[suit];
  const int n = info.slots;
  Analysis a;
  a.width = image.width;
  a.height = image.height;
  const size_t count = size_t(image.width) * size_t(image.height);
  a.hsv.resize(count);
  a.coloured.resize(count);
  a.nearest.resize(count);
  a.slot.assign(count, -1);
  a.hard.assign(count, 0);
  std::vector<int> label(count);
  std::vector<uint8_t> in(count);
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* t = &image.rgba[i * 4];
    const Hsv hsv = ToHsv(t[0] / 255.0, t[1] / 255.0, t[2] / 255.0);
    a.hsv[i] = hsv;
    const double opaque = t[3] > 127 ? 1.0 : 0.0;
    a.coloured[i] = info.glowOnly ? Smooth(hsv.s, 0.45, 0.55) * Smooth(hsv.v, 0.25, 0.35) * opaque
                                  : Smooth(hsv.s, 0.12, 0.25) * Smooth(hsv.v, 0.08, 0.16) * opaque;
    int best = 0;
    double bestDistance = 1e9;
    for (int j = 0; j < n; ++j) {
      const double d = HueDistance(hsv.h, info.medians[j].h);
      if (d < bestDistance) {
        bestDistance = d, best = j;
      }
    }
    label[i] = best;
    a.nearest[i] = bestDistance;
    in[i] = a.coloured[i] > 0.5 && bestDistance < kGateZero;
  }

  // Majority of the coloured texels round each, ties kept as they are.
  const int k = image.width >= 256 ? 5 : 3;
  std::vector<std::vector<int>> votes(n);
  for (int j = 0; j < n; ++j) {
    std::vector<uint8_t> mask(count);
    for (size_t i = 0; i < count; ++i) {
      mask[i] = in[i] && label[i] == j;
    }
    votes[j] = BoxSum(mask, image.width, image.height, k);
  }
  std::vector<int> cleaned(count);
  for (size_t i = 0; i < count; ++i) {
    int best = 0, top = -1, second = -1;
    for (int j = 0; j < n; ++j) {
      const int v = votes[j][i];
      if (v > top) {
        second = top, top = v, best = j;
      } else if (v > second) {
        second = v;
      }
    }
    cleaned[i] = (top <= 0 || (n > 1 && top == second)) ? label[i] : best;
  }

  // Islands: 4-connected coloured texels between dark seams. One that is
  // mostly one slot (or small) is that slot, near enough in hue.
  std::vector<int> island(count, -1);
  std::vector<size_t> stack;
  for (size_t start = 0; start < count; ++start) {
    if (!in[start] || a.hsv[start].v < kSeamValue || island[start] >= 0) {
      continue;
    }
    std::vector<int> tally(n, 0);
    std::vector<size_t> members;
    const int id = int(start);
    island[start] = id;
    stack.push_back(start);
    while (!stack.empty()) {
      const size_t p = stack.back();
      stack.pop_back();
      members.push_back(p);
      ++tally[cleaned[p]];
      const int x = int(p % image.width), y = int(p / image.width);
      const auto visit = [&](int qx, int qy) {
        if (qx < 0 || qy < 0 || qx >= image.width || qy >= image.height) {
          return;
        }
        const size_t q = size_t(qy) * image.width + qx;
        if (in[q] && a.hsv[q].v >= kSeamValue && island[q] < 0) {
          island[q] = id;
          stack.push_back(q);
        }
      };
      visit(x, y - 1);
      visit(x, y + 1);
      visit(x - 1, y);
      visit(x + 1, y);
    }
    const int best = int(std::max_element(tally.begin(), tally.end()) - tally.begin());
    const double share = double(tally[best]) / double(members.size());
    if (share < kIslandShare && int(members.size()) >= kIslandMin) {
      continue;
    }
    for (const size_t p : members) {
      if (HueDistance(a.hsv[p].h, info.medians[best].h) < kIslandHue) {
        cleaned[p] = best;
        a.hard[p] = 1;
      }
    }
  }
  for (size_t i = 0; i < count; ++i) {
    if (in[i]) {
      a.slot[i] = cleaned[i];
    }
  }
  return a;
}

}  // namespace

const Suit& SuitInfo(int suit) { return kSuits[std::clamp(suit, 0, kSuitCount - 1)]; }

uint32_t DefaultColour(int suit, int slot) {
  const Hsv& m = SuitInfo(suit).medians[std::clamp(slot, 0, kMaxSlots - 1)];
  double rgb[3];
  FromHsv(m.h, m.s, m.v, rgb);
  uint32_t out = 0;
  for (const double c : rgb) {
    out = (out << 8) | uint32_t(std::clamp(std::lround(c), 0l, 255l));
  }
  return out;
}

std::optional<uint32_t> Colour(int suit, int slot) {
  if (suit < 0 || suit >= kSuitCount || slot < 0 || slot >= kSuits[suit].slots) {
    return std::nullopt;
  }
  std::lock_guard<std::mutex> lock(sMutex);
  return sPicks[suit][slot];
}

void SetColour(int suit, int slot, std::optional<uint32_t> rgb) {
  if (suit < 0 || suit >= kSuitCount || slot < 0 || slot >= kSuits[suit].slots) {
    return;
  }
  std::lock_guard<std::mutex> lock(sMutex);
  sPicks[suit][slot] = rgb ? std::optional<uint32_t>(*rgb & 0xFFFFFFu) : std::nullopt;
}

Picks SuitPicks(int suit) {
  if (suit < 0 || suit >= kSuitCount) {
    return {};
  }
  std::lock_guard<std::mutex> lock(sMutex);
  return sPicks[suit];
}

void Randomise(int suit, uint32_t seed) {
  if (suit < 0 || suit >= kSuitCount) {
    return;
  }
  std::mt19937 random(seed);
  std::uniform_real_distribution<double> unit(0.0, 1.0);
  const double start = unit(random) * 360.0;
  // The golden angle apart, jittered, so the slots never share a hue.
  for (int slot = 0; slot < kSuits[suit].slots; ++slot) {
    const double h = start + slot * 137.5 + (unit(random) - 0.5) * 30.0;
    const double s = 0.55 + 0.45 * unit(random);
    const double v = 0.6 + 0.4 * unit(random);
    double rgb[3];
    FromHsv(h, s, v, rgb);
    uint32_t value = 0;
    for (const double c : rgb) {
      value = (value << 8) | uint32_t(std::clamp(std::lround(c), 0l, 255l));
    }
    SetColour(suit, slot, value);
  }
}

std::string FormatColour(std::optional<uint32_t> rgb) {
  if (!rgb) {
    return {};
  }
  char text[8];
  std::snprintf(text, sizeof(text), "%06X", unsigned(*rgb & 0xFFFFFFu));
  return text;
}

std::optional<uint32_t> ParseColour(const std::string& text) {
  size_t at = !text.empty() && text[0] == '#' ? 1 : 0;
  if (text.size() - at != 6) {
    return std::nullopt;
  }
  uint32_t value = 0;
  for (; at < text.size(); ++at) {
    const char c = text[at];
    const int digit = c >= '0' && c <= '9'   ? c - '0'
                      : c >= 'a' && c <= 'f' ? c - 'a' + 10
                      : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                             : -1;
    if (digit < 0) {
      return std::nullopt;
    }
    value = (value << 4) | uint32_t(digit);
  }
  return value;
}

std::string PicksKey(int suit) { return PicksKey(suit, SuitPicks(suit)); }

std::string PicksKey(int suit, const Picks& picks) {
  std::string key;
  bool any = false;
  for (int j = 0; j < SuitInfo(suit).slots; ++j) {
    key += (j ? "," : "") + (picks[j] ? FormatColour(picks[j]) : std::string("-"));
    any |= picks[j].has_value();
  }
  return any ? key : std::string();
}

void SetPicksKey(int suit, const std::string& key) {
  if (suit < 0 || suit >= kSuitCount) {
    return;
  }
  size_t at = 0;
  for (int j = 0; j < kSuits[suit].slots; ++j) {
    const size_t begin = std::min(at, key.size());
    const size_t end = std::min(key.find(',', begin), key.size());
    SetColour(suit, j, ParseColour(key.substr(begin, end - begin)));
    at = end + 1;
  }
}

void SetServed(const std::array<std::string, kSuitCount>& served) {
  std::lock_guard<std::mutex> lock(sMutex);
  sServed = served;
}

bool Pending(bool wanted) {
  std::array<std::string, kSuitCount> served;
  {
    std::lock_guard<std::mutex> lock(sMutex);
    served = sServed;
  }
  for (int suit = 0; suit < kSuitCount; ++suit) {
    if ((wanted ? PicksKey(suit) : std::string()) != served[suit]) {
      return true;
    }
  }
  return false;
}

size_t Recolour(Image& image, int suit, const Picks& picks) {
  if (suit < 0 || suit >= kSuitCount || image.width <= 0 || image.height <= 0 ||
      image.rgba.size() != size_t(image.width) * size_t(image.height) * 4) {
    return 0;
  }
  const Suit& info = kSuits[suit];
  const Analysis a = Analyse(image, suit);
  const int n = info.slots;
  const size_t count = a.hsv.size();

  // Each slot's share of a texel: the 3x3 mix of slots round it, so a border
  // blends; one-hot where a whole island decided it.
  std::vector<uint8_t> any(count);
  for (size_t i = 0; i < count; ++i) {
    any[i] = a.slot[i] >= 0;
  }
  const std::vector<int> around = BoxSum(any, a.width, a.height, 3);
  std::vector<std::vector<int>> share(n);
  for (int j = 0; j < n; ++j) {
    std::vector<uint8_t> mask(count);
    for (size_t i = 0; i < count; ++i) {
      mask[i] = a.slot[i] == j;
    }
    share[j] = BoxSum(mask, a.width, a.height, 3);
  }

  std::array<Hsv, kMaxSlots> targets{};
  bool anyPick = false;
  for (int j = 0; j < n; ++j) {
    if (picks[j]) {
      const uint32_t c = *picks[j];
      targets[j] = ToHsv(((c >> 16) & 255) / 255.0, ((c >> 8) & 255) / 255.0, (c & 255) / 255.0);
      anyPick = true;
    }
  }
  if (!anyPick) {
    return 0;
  }

  size_t changed = 0;
  for (size_t i = 0; i < count; ++i) {
    const Hsv& t = a.hsv[i];
    double gate = (1.0 - Smooth(a.nearest[i], kGateFull, kGateZero)) * a.coloured[i];
    if (info.warm) {
      gate *= 1.0 - Smooth(t.h, 70.0, 75.0) * (1.0 - Smooth(t.h, 200.0, 205.0));
    }
    if (gate <= 0.0) {
      continue;
    }
    uint8_t* texel = &image.rgba[i * 4];
    const double original[3] = {double(texel[0]), double(texel[1]), double(texel[2])};
    // A soft-edge texel outside every slot moves with its nearest slot.
    std::array<double, kMaxSlots> weight{};
    if (a.slot[i] < 0 || a.hard[i]) {
      int nearest = a.slot[i];
      if (nearest < 0) {
        double best = 1e9;
        for (int j = 0; j < n; ++j) {
          const double d = HueDistance(t.h, info.medians[j].h);
          if (d < best) {
            best = d, nearest = j;
          }
        }
      }
      weight[nearest] = 1.0;
    } else {
      double sum = 0.0;
      for (int j = 0; j < n; ++j) {
        weight[j] = double(share[j][i]) / double(std::max(around[i], 1));
        sum += weight[j];
      }
      for (int j = 0; j < n; ++j) {
        weight[j] = sum > 0.0 ? weight[j] / sum : 0.0;
      }
    }
    double out[3] = {0.0, 0.0, 0.0};
    for (int j = 0; j < n; ++j) {
      if (weight[j] <= 0.0) {
        continue;
      }
      double rgb[3] = {original[0], original[1], original[2]};
      if (picks[j]) {
        const Hsv& m = info.medians[j];
        const Hsv& to = targets[j];
        const double h = to.h + kCompress * Wrap(t.h - m.h);
        const double s = std::clamp(t.s * std::min(kScaleMax, to.s / std::max(m.s, 1e-3)), 0.0, 1.0);
        const double v = std::clamp(t.v * std::min(kScaleMax, to.v / std::max(m.v, 1e-3)), 0.0, 1.0);
        FromHsv(h, s, v, rgb);
      }
      for (int c = 0; c < 3; ++c) {
        out[c] += weight[j] * rgb[c];
      }
    }
    bool differs = false;
    for (int c = 0; c < 3; ++c) {
      const double mixed = gate * out[c] + (1.0 - gate) * original[c];
      const uint8_t byte = uint8_t(std::clamp(std::floor(mixed + 0.5), 0.0, 255.0));
      differs |= byte != texel[c];
      texel[c] = byte;
    }
    changed += differs;
  }
  return changed;
}

// --- CMPR TXTRs ------------------------------------------------------------------------------

namespace {

uint32_t ReadBE16(const uint8_t* p) { return (uint32_t(p[0]) << 8) | p[1]; }
uint32_t ReadBE32(const uint8_t* p) { return (ReadBE16(p) << 16) | ReadBE16(p + 2); }

void PutBE16(std::vector<uint8_t>& out, uint32_t v) {
  out.push_back(uint8_t(v >> 8));
  out.push_back(uint8_t(v));
}

void PutBE32(std::vector<uint8_t>& out, uint32_t v) {
  PutBE16(out, v >> 16);
  PutBE16(out, v & 0xFFFF);
}

uint8_t Expand5(uint32_t v) { return uint8_t((v << 3) | (v >> 2)); }
uint8_t Expand6(uint32_t v) { return uint8_t((v << 2) | (v >> 4)); }

uint32_t To565(const int* c) {
  const auto q = [](int v, int bits) { return uint32_t(std::clamp((v * ((1 << bits) - 1) + 127) / 255, 0, (1 << bits) - 1)); };
  return (q(c[0], 5) << 11) | (q(c[1], 6) << 5) | q(c[2], 5);
}

// The four colours a block's two endpoints make (as the decoder does).
void Palette(uint32_t c0, uint32_t c1, int out[4][4]) {
  const int e[2][3] = {{Expand5(c0 >> 11), Expand6((c0 >> 5) & 63), Expand5(c0 & 31)},
                       {Expand5(c1 >> 11), Expand6((c1 >> 5) & 63), Expand5(c1 & 31)}};
  for (int c = 0; c < 3; ++c) {
    out[0][c] = e[0][c];
    out[1][c] = e[1][c];
    if (c0 > c1) {
      out[2][c] = (5 * e[0][c] + 3 * e[1][c]) >> 3;
      out[3][c] = (3 * e[0][c] + 5 * e[1][c]) >> 3;
    } else {
      out[2][c] = (e[0][c] + e[1][c]) >> 1;
      out[3][c] = out[2][c];
    }
  }
  for (int i = 0; i < 4; ++i) {
    out[i][3] = (i == 3 && c0 <= c1) ? 0 : 255;
  }
}

// One 4x4 block from 16 RGBA texels: endpoints from the colour box's diagonal
// (the two corners that are furthest apart along the texels' main axis), indices
// by nearest palette colour. A block with a transparent texel uses the 3-colour
// mode, whose index 3 is transparent.
void EncodeBlock(const uint8_t px[16][4], uint8_t* out) {
  bool transparent = false;
  int lo[3] = {255, 255, 255}, hi[3] = {0, 0, 0};
  double mean[3] = {0, 0, 0};
  int opaque = 0;
  for (int i = 0; i < 16; ++i) {
    if (px[i][3] < 128) {
      transparent = true;
      continue;
    }
    ++opaque;
    for (int c = 0; c < 3; ++c) {
      lo[c] = std::min<int>(lo[c], px[i][c]);
      hi[c] = std::max<int>(hi[c], px[i][c]);
      mean[c] += px[i][c];
    }
  }
  if (opaque == 0) {
    const uint8_t clear[8] = {0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF};
    std::copy(clear, clear + 8, out);
    return;
  }
  for (double& m : mean) {
    m /= opaque;
  }
  // The box diagonal's direction by the sign of each channel's correlation with
  // the first channel's spread: which corner pair spans the texels.
  int sign[3] = {1, 1, 1};
  for (int c = 1; c < 3; ++c) {
    double cov = 0;
    for (int i = 0; i < 16; ++i) {
      if (px[i][3] >= 128) {
        cov += (px[i][0] - mean[0]) * (px[i][c] - mean[c]);
      }
    }
    sign[c] = cov < 0 ? -1 : 1;
  }
  int a[3], b[3];
  for (int c = 0; c < 3; ++c) {
    a[c] = sign[c] > 0 ? lo[c] : hi[c];
    b[c] = sign[c] > 0 ? hi[c] : lo[c];
  }
  uint32_t c0 = To565(a), c1 = To565(b);
  if (transparent) {
    if (c0 > c1) {
      std::swap(c0, c1);
    }
  } else {
    if (c0 < c1) {
      std::swap(c0, c1);
    }
    if (c0 == c1) {
      // One colour: keep the 4-colour mode (c0 > c1) with the endpoints a step apart.
      c0 > 0 ? --c1 : ++c0;
    }
  }
  int palette[4][4];
  Palette(c0, c1, palette);
  out[0] = uint8_t(c0 >> 8), out[1] = uint8_t(c0), out[2] = uint8_t(c1 >> 8), out[3] = uint8_t(c1);
  for (int y = 0; y < 4; ++y) {
    uint8_t row = 0;
    for (int x = 0; x < 4; ++x) {
      const uint8_t* t = px[y * 4 + x];
      int best = 0;
      if (transparent && t[3] < 128) {
        best = 3;
      } else {
        int bestDistance = 1 << 30;
        for (int i = 0; i < (transparent ? 3 : 4); ++i) {
          const int d0 = t[0] - palette[i][0], d1 = t[1] - palette[i][1], d2 = t[2] - palette[i][2];
          const int d = d0 * d0 + d1 * d1 + d2 * d2;
          if (d < bestDistance) {
            bestDistance = d, best = i;
          }
        }
      }
      row = uint8_t((row << 2) | best);
    }
    out[4 + y] = row;
  }
}

// One level as 8x8 tiles of four blocks; edges past the image repeat.
void EncodeLevel(const Image& image, std::vector<uint8_t>& out) {
  for (int ty = 0; ty < image.height; ty += 8) {
    for (int tx = 0; tx < image.width; tx += 8) {
      for (int block = 0; block < 4; ++block) {
        uint8_t px[16][4];
        for (int y = 0; y < 4; ++y) {
          for (int x = 0; x < 4; ++x) {
            const int sx = std::min(tx + (block & 1) * 4 + x, image.width - 1);
            const int sy = std::min(ty + (block >> 1) * 4 + y, image.height - 1);
            std::copy_n(&image.rgba[(size_t(sy) * image.width + sx) * 4], 4, px[y * 4 + x]);
          }
        }
        uint8_t bytes[8];
        EncodeBlock(px, bytes);
        out.insert(out.end(), bytes, bytes + 8);
      }
    }
  }
}

Image Halve(const Image& in) {
  Image out;
  out.width = std::max(in.width / 2, 1);
  out.height = std::max(in.height / 2, 1);
  out.rgba.resize(size_t(out.width) * out.height * 4);
  for (int y = 0; y < out.height; ++y) {
    for (int x = 0; x < out.width; ++x) {
      int sum[4] = {0, 0, 0, 0}, weight = 0;
      for (int dy = 0; dy < 2; ++dy) {
        for (int dx = 0; dx < 2; ++dx) {
          const int sx = std::min(x * 2 + dx, in.width - 1), sy = std::min(y * 2 + dy, in.height - 1);
          const uint8_t* t = &in.rgba[(size_t(sy) * in.width + sx) * 4];
          // Transparent texels do not bleed their colour in.
          const int w = t[3] >= 128 ? 1 : 0;
          for (int c = 0; c < 3; ++c) {
            sum[c] += t[c] * w;
          }
          sum[3] += t[3];
          weight += w;
        }
      }
      uint8_t* o = &out.rgba[(size_t(y) * out.width + x) * 4];
      for (int c = 0; c < 3; ++c) {
        o[c] = weight > 0 ? uint8_t((sum[c] + weight / 2) / weight) : 0;
      }
      o[3] = uint8_t(sum[3] / 4);
    }
  }
  return out;
}

}  // namespace

bool DecodeCmprTxtr(const std::vector<uint8_t>& txtr, Image& out) {
  // Header: u32 format, u16 width, u16 height, u32 mip count; then the levels.
  if (txtr.size() < 12 || ReadBE32(txtr.data()) != 10) {
    return false;
  }
  const int width = int(ReadBE16(txtr.data() + 4));
  const int height = int(ReadBE16(txtr.data() + 6));
  if (width == 0 || height == 0 || width % 8 != 0 || height % 8 != 0 ||
      txtr.size() < 12 + size_t(width) * size_t(height) / 2) {
    return false;
  }
  out = {};
  out.width = width;
  out.height = height;
  out.rgba.assign(size_t(width) * size_t(height) * 4, 0);
  // 8x8 tiles, each four 4x4 blocks: two RGB565 colours, then 2-bit indices a
  // row per byte, the leftmost texel in the top bits.
  const uint8_t* src = txtr.data() + 12;
  for (int ty = 0; ty < height; ty += 8) {
    for (int tx = 0; tx < width; tx += 8) {
      for (int block = 0; block < 4; ++block, src += 8) {
        int palette[4][4];
        Palette(ReadBE16(src), ReadBE16(src + 2), palette);
        const int bx = tx + (block & 1) * 4;
        const int by = ty + (block >> 1) * 4;
        for (int y = 0; y < 4; ++y) {
          for (int x = 0; x < 4; ++x) {
            const int* colour = palette[(src[4 + y] >> (6 - 2 * x)) & 3];
            uint8_t* o = &out.rgba[(size_t(by + y) * width + (bx + x)) * 4];
            for (int c = 0; c < 4; ++c) {
              o[c] = uint8_t(colour[c]);
            }
          }
        }
      }
    }
  }
  return true;
}

std::vector<uint8_t> EncodeCmprTxtr(const Image& image, uint32_t mipCount) {
  std::vector<uint8_t> out;
  PutBE32(out, 10);
  PutBE16(out, uint32_t(image.width));
  PutBE16(out, uint32_t(image.height));
  PutBE32(out, std::max<uint32_t>(mipCount, 1));
  Image level = image;
  for (uint32_t i = 0; i < std::max<uint32_t>(mipCount, 1); ++i) {
    EncodeLevel(level, out);
    if (level.width == 1 && level.height == 1) {
      break;
    }
    level = Halve(level);
  }
  return out;
}

}  // namespace PortSuitColours
