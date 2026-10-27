// Host test of the Trilogy save.bin container (platform/port_save_trilogy.cpp) against real save.bins
// from the Prime 1 port's fixture set, copied to tests/fixtures/trilogy/. Argument: that directory.
#include "port_save_trilogy.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace PortSave::Trilogy;

namespace {
int sFailures = 0;
void Check(bool ok, const char* what, const std::string& file) {
  if (!ok) {
    std::fprintf(stderr, "FAIL %s: %s\n", file.c_str(), what);
    ++sFailures;
  }
}
std::vector<uint8_t> Load(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <fixture dir>\n", argv[0]);
    return 2;
  }
  int withEchoes = 0;
  for (const char* name : {"01-firstboot", "12-gb-packed", "13-gbatemp-100", "21-pal-marco-100", "23-pal-toxico-perfect"}) {
    const std::string path = std::string(argv[1]) + "/" + name + ".bin";
    const std::vector<uint8_t> bin = Load(path);
    std::string error;
    Check(!bin.empty(), "fixture missing", path);
    if (bin.empty()) {
      continue;
    }
    Check(Validate(bin.data(), bin.size(), error), error.c_str(), name);
    for (int f = 0; f < int(kFiles); ++f) {
      Payload p;
      if (!Read(bin.data(), f, p)) {
        continue;
      }
      ++withEchoes;
      // Writing a file's own payload back changes nothing: slot, summary, constant and CRC agree.
      std::vector<uint8_t> out;
      Check(Write(bin, f, p, out, error), error.c_str(), name);
      Check(out == bin, "own payload written back is not byte-identical", name);
      // And into another file the result is still a valid save.bin that reads back the same.
      for (int g = 0; g < int(kFiles); ++g) {
        if (g == f) {
          continue;
        }
        std::vector<uint8_t> moved;
        Check(Write(bin, g, p, moved, error), error.c_str(), name);
        Check(Validate(moved.data(), moved.size(), error), "moved save fails validation", name);
        Payload q;
        Check(Read(moved.data(), g, q) && !std::memcmp(q.partA, p.partA, kPartSize) &&
                  !std::memcmp(q.partB, p.partB, kPartSize) && q.summary.mlvl == p.summary.mlvl &&
                  q.summary.tanks == p.summary.tanks && q.summary.percent == p.summary.percent &&
                  q.summary.playTime == p.summary.playTime,
              "payload does not read back", name);
        // The other games' parts are untouched: only the slot, summary line, entry and CRC change.
        size_t outside = 0;
        for (size_t i = 4; i < bin.size(); ++i) {
          const bool inSlot = i >= kSlotBase + kSlotSize * g && i < kSlotBase + kSlotSize * (g + 1);
          if (moved[i] != bin[i] && !inSlot) {
            ++outside;
          }
        }
        Check(outside <= 7 + kSummaryBytes + 4, "a write changed bytes outside Echoes' file", name);
      }
      // A flipped byte in the checksummed range is caught.
      std::vector<uint8_t> bad = bin;
      bad[0x100] ^= 1;
      Check(!Validate(bad.data(), bad.size(), error), "a corrupted save.bin validates", name);
      break;
    }
  }
  Check(withEchoes > 0, "no fixture has Echoes progress; the test proved nothing", "fixtures");
  std::printf("%d fixture file(s) with Echoes progress, %d failure(s)\n", withEchoes, sFailures);
  return sFailures == 0 ? 0 : 1;
}
