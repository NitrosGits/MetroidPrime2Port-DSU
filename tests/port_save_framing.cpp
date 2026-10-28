#include "port_save_framing.h"
#include "port_test.h"

#include <algorithm>
#include <array>
#include <cstring>

using Block = std::array<uchar, 0x2000>;
namespace F = PortSave::Framing;

// Independent console reader: no production decoding or CRC routines.
static uint ConsoleWord(const uchar* p) {
  return uint(p[0]) * 0x1000000u + uint(p[1]) * 0x10000u + uint(p[2]) * 0x100u + p[3];
}

static uint ConsoleCrc(const uchar* p, size_t n) {
  uint crc = 0xFFFFFFFF;
  while (n--) {
    crc ^= *p++;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0);
    }
  }
  return crc;
}

static void Store(uchar* p, uint v, bool little) {
  for (int i = 0; i < 4; ++i) {
    p[i] = uchar(v >> (8 * (little ? i : 3 - i)));
  }
}

static Block Fixture(uint generation, bool little) {
  Block b{};
  for (size_t i = 8; i < b.size(); ++i) {
    b[i] = uchar(i * 37 + i / 256);
  }
  Store(b.data() + 4, generation, little);
  Store(b.data(), ConsoleCrc(b.data() + 4, b.size() - 4), little);
  return b;
}

int main() {
  CHECK_EQ(ConsoleCrc(reinterpret_cast<const uchar*>("123456789"), 9), 0x340BC6D9u);
  CHECK_EQ(CCRC32::Calculate("123456789", 9, 0xFFFFFFFF), 0x340BC6D9u);
  for (bool little : {false, true}) {
    for (uint generation : {0u, 1u}) {
      Block b = Fixture(generation, little);
      const Block original = b;
      uint read = 99;
      CHECK(F::CheckBlockCrc(b.data(), b.size()));
      CHECK(F::ReadSlot(b.data(), b.size(), read));
      CHECK_EQ(read, generation);
      CHECK(b == original); // Reading a shared/legacy save must not rewrite it.

      // A subsequent write migrates only the framing to the console format.
      F::FinishSlot(b.data(), b.size(), 1 - read);
      CHECK(std::equal(b.begin() + 8, b.end(), original.begin() + 8));
      CHECK_EQ(ConsoleWord(b.data() + 4), 1 - generation);
      CHECK_EQ(ConsoleWord(b.data()), ConsoleCrc(b.data() + 4, b.size() - 4));
      CHECK(F::ReadSlot(b.data(), b.size(), read));
      CHECK_EQ(read, 1 - generation);

      // Repeated alternating writes remain readable in both programs.
      for (uint i = 0; i < 6; ++i) {
        F::FinishSlot(b.data(), b.size(), i & 1);
        CHECK_EQ(ConsoleWord(b.data()), ConsoleCrc(b.data() + 4, b.size() - 4));
        CHECK(F::ReadSlot(b.data(), b.size(), read));
        CHECK_EQ(read, i & 1);
      }

      // Header CRC accepts either old order, but all new headers are big-endian.
      b = original;
      F::FinishBlock(b.data(), b.size());
      CHECK(std::equal(b.begin() + 4, b.end(), original.begin() + 4));
      CHECK_EQ(ConsoleWord(b.data()), ConsoleCrc(b.data() + 4, b.size() - 4));
      CHECK(F::CheckBlockCrc(b.data(), b.size()));

      b = original;
      b[1024] ^= 1;
      CHECK(!F::CheckBlockCrc(b.data(), b.size()));
      CHECK(!F::ReadSlot(b.data(), b.size(), read));
      b = original;
      b[0] ^= 1;
      CHECK(!F::ReadSlot(b.data(), b.size(), read));
    }
    // Even a correct CRC cannot make an invalid slot selector safe.
    for (uint generation : {2u, 0x01000000u, 0xFFFFFFFFu}) {
      Block b = Fixture(generation, little);
      uint read = 0;
      CHECK(F::CheckBlockCrc(b.data(), b.size()));
      CHECK(!F::ReadSlot(b.data(), b.size(), read));
    }
  }
  uint read = 0;
  CHECK(!F::ReadSlot(nullptr, 0, read));
  CHECK(!F::CheckBlockCrc(nullptr, 0));
  Block b{};
  CHECK(!F::ReadSlot(b.data(), 7, read));
  CHECK(!F::CheckBlockCrc(b.data(), 3));

  // CRC words can coincidentally be byte-palindromic. Decode the generation
  // using its valid 0/1 range, rather than blindly preferring one byte order.
  b = Fixture(1, true);
  Store(b.data() + 8, 268169, false);
  const uint crc = ConsoleCrc(b.data() + 4, b.size() - 4);
  CHECK_EQ(crc, 0x3FE8E83Fu);
  Store(b.data(), crc, true);
  CHECK(F::ReadSlot(b.data(), b.size(), read));
  CHECK_EQ(read, 1u);
  return PORT_TEST_RESULT();
}
