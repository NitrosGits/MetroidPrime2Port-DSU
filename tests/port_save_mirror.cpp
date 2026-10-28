#include "port_save_mirror.h"
#include "port_test.h"

#include <string>
#include <string_view>

int main() {
  using PortSave::Mirror::IsCardForRelease;
  for (std::string_view mounted : {"G2ME", "G2MP", "R3ME", "R3MP"}) {
    for (std::string_view saved : {"G2ME", "G2MP", "R3ME", "R3MP"}) {
      const std::string header = std::string(saved) + "01";
      CHECK_EQ(IsCardForRelease(header, mounted), saved == mounted);
      CHECK(!IsCardForRelease(std::string(saved) + "99", mounted));
      for (size_t size = 0; size < 6; ++size) {
        CHECK(!IsCardForRelease(std::string_view(header.data(), size), mounted));
      }
    }
    CHECK(!IsCardForRelease("GM8E01", mounted));
    CHECK(!IsCardForRelease("R3MJ01", mounted));
    CHECK(!IsCardForRelease("g2me01", mounted));
    CHECK(!IsCardForRelease(std::string_view("R3ME\0\0", 6), mounted));
  }
  CHECK(!IsCardForRelease("GM8E01", "GM8E"));
  CHECK(!IsCardForRelease("R3ME01", "R3M"));
  CHECK(!IsCardForRelease("R3ME01", "R3ME01"));
  CHECK(!IsCardForRelease("R3ME01", ""));
  return PORT_TEST_RESULT();
}
