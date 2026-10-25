// Suit colour customisation's recolour on synthetic textures, and the picks'
// settings text.

#include "port_suit_colours.h"
#include "port_test.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

using PortSuitColours::Image;
namespace PSC = PortSuitColours;

constexpr int kVaria = 0, kDark = 1, kLight = 2;

void Rgb(double h, double s, double v, uint8_t* out) {
  h = std::fmod(h, 360.0) / 60.0;
  const double c = v * s, x = c * (1.0 - std::fabs(std::fmod(h, 2.0) - 1.0)), m = v - c;
  double r = 0, g = 0, b = 0;
  switch (int(h)) {
  case 0: r = c, g = x; break;
  case 1: r = x, g = c; break;
  case 2: g = c, b = x; break;
  case 3: g = x, b = c; break;
  case 4: r = x, b = c; break;
  default: r = c, b = x; break;
  }
  out[0] = uint8_t(std::lround((r + m) * 255));
  out[1] = uint8_t(std::lround((g + m) * 255));
  out[2] = uint8_t(std::lround((b + m) * 255));
  out[3] = 255;
}

double Hue(const uint8_t* t) {
  const double r = t[0] / 255.0, g = t[1] / 255.0, b = t[2] / 255.0;
  const double max = std::max({r, g, b}), range = max - std::min({r, g, b});
  if (range <= 0) {
    return 0;
  }
  const double h = max == r ? std::fmod((g - b) / range + 6, 6) : max == g ? (b - r) / range + 2 : (r - g) / range + 4;
  return h * 60;
}

uint8_t* At(Image& image, int x, int y) { return &image.rgba[(size_t(y) * image.width + x) * 4]; }

// A Varia Suit texture: orange on the left, red on the right, with a cyan
// patch in the orange and a grey patch in the red.
Image VariaTexture() {
  Image image;
  image.width = image.height = 64;
  image.rgba.resize(64 * 64 * 4);
  for (int y = 0; y < 64; ++y) {
    for (int x = 0; x < 64; ++x) {
      uint8_t* t = At(image, x, y);
      if (x >= 8 && x < 16 && y >= 8 && y < 16) {
        Rgb(180, 0.8, 0.9, t);
      } else if (x >= 48 && x < 56 && y >= 8 && y < 16) {
        Rgb(0, 0.0, 0.6, t);
      } else if (x < 32) {
        Rgb(39 + (y % 5) - 2, 0.82, 0.9, t);
      } else {
        Rgb(5, 0.9, 0.84, t);
      }
    }
  }
  return image;
}

void TestRecolour() {
  Image image = VariaTexture();
  const Image original = image;
  PSC::Picks picks{};
  CHECK_EQ(PSC::Recolour(image, kVaria, picks), size_t(0));
  CHECK(image.rgba == original.rgba);

  picks[0] = 0x2050FF;  // blue primary, secondary left as it is
  const size_t changed = PSC::Recolour(image, kVaria, picks);
  CHECK(changed > 1000);
  // The orange is blue now, its spread of hues kept (compressed).
  for (const int y : {20, 40, 60}) {
    const double hue = Hue(At(image, 20, y));
    CHECK_MSG(std::fabs(hue - 225.0) < 6.0, "orange texel hue " + std::to_string(hue));
  }
  // Red, the visor and grey stay.
  for (const auto& [x, y] : {std::pair{40, 30}, {60, 60}, {12, 12}, {52, 12}}) {
    CHECK(std::equal(At(image, x, y), At(image, x, y) + 4, &original.rgba[(size_t(y) * 64 + x) * 4]));
  }
}

// A CMPR TXTR survives decode, encode and decode again to within the format's
// error, and a recolour keeps the size.
void TestCmpr() {
  Image image = VariaTexture();
  const std::vector<uint8_t> txtr = PSC::EncodeCmprTxtr(image, 4);
  CHECK_EQ(txtr.size(), size_t(12 + 64 * 64 / 2 + 32 * 32 / 2 + 16 * 16 / 2 + 8 * 8 / 2));
  CHECK_EQ(txtr[3], uint8_t(10));
  Image decoded;
  CHECK(PSC::DecodeCmprTxtr(txtr, decoded));
  CHECK_EQ(decoded.width, 64);
  int worst = 0;
  for (size_t i = 0; i < image.rgba.size(); ++i) {
    worst = std::max(worst, std::abs(int(image.rgba[i]) - int(decoded.rgba[i])));
  }
  CHECK_MSG(worst < 40, "worst channel error " + std::to_string(worst));
  CHECK(!PSC::DecodeCmprTxtr({1, 2, 3}, decoded));
}

void TestPicks() {
  CHECK(PSC::ParseColour("#ff00aa") == std::optional<uint32_t>(0xFF00AA));
  CHECK(PSC::ParseColour("00FF00") == std::optional<uint32_t>(0x00FF00));
  CHECK(!PSC::ParseColour("-"));
  CHECK(!PSC::ParseColour("12345G"));
  CHECK_EQ(PSC::FormatColour(0x0A0B0C), std::string("0A0B0C"));

  CHECK_EQ(PSC::PicksKey(kVaria), std::string());
  PSC::SetPicksKey(kVaria, "2050FF,-");
  CHECK(PSC::Colour(kVaria, 0) == std::optional<uint32_t>(0x2050FF));
  CHECK(!PSC::Colour(kVaria, 1));
  CHECK_EQ(PSC::PicksKey(kVaria), std::string("2050FF,-"));
  CHECK(!PSC::Colour(kVaria, 2));  // Varia has two slots
  PSC::SetPicksKey(kDark, "FF0000,ABCDEF");
  CHECK_EQ(PSC::PicksKey(kDark), std::string("FF0000"));  // Dark has one slot

  std::array<std::string, PSC::kSuitCount> served{};
  CHECK(PSC::Pending(true));
  CHECK(!PSC::Pending(false));  // nothing served, nothing wanted
  served[kVaria] = "2050FF,-";
  served[kDark] = "FF0000";
  PSC::SetServed(served);
  CHECK(!PSC::Pending(true));
  CHECK(PSC::Pending(false));
  PSC::SetPicksKey(kVaria, "");
  PSC::SetPicksKey(kDark, "");
  CHECK_EQ(PSC::PicksKey(kVaria), std::string());
  CHECK(PSC::Pending(true));
  PSC::SetServed({});
  CHECK(!PSC::Pending(true));

  PSC::Randomise(kVaria, 1234);
  const std::optional<uint32_t> a = PSC::Colour(kVaria, 0), b = PSC::Colour(kVaria, 1);
  CHECK(a && b && *a != *b);
  PSC::Randomise(kVaria, 1234);
  CHECK(PSC::Colour(kVaria, 0) == a);  // same seed, same picks
  PSC::SetPicksKey(kVaria, "");
  PSC::Randomise(kLight, 1);
  CHECK(PSC::Colour(kLight, 0).has_value());
  PSC::SetPicksKey(kLight, "");

  for (int suit = 0; suit < PSC::kSuitCount; ++suit) {
    const PSC::Suit& info = PSC::SuitInfo(suit);
    CHECK(info.slots >= 1 && info.slots <= PSC::kMaxSlots);
    for (int slot = 0; slot < info.slots; ++slot) {
      CHECK(info.slotKeys[slot] != nullptr && info.slotNames[slot] != nullptr);
    }
  }
  CHECK_EQ(PSC::DefaultColour(kVaria, 0) >> 16, uint32_t(230));  // orange: V 0.902
}

}  // namespace

int main() {
  TestRecolour();
  TestCmpr();
    TestPicks();
  if (port_test::Failures() == 0) {
    std::printf("port_suit_colours: all passed\n");
  }
  return PORT_TEST_RESULT();
}
