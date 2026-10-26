#include "port_mods_disc.h"
#include "port_test.h"

#include <map>
#include <tuple>

namespace {
struct Entry {
  uint32_t number;
  bool directory;
  std::string name;
};
std::map<std::string, std::vector<Entry>> tree;
std::vector<std::string> opened;
std::map<DVDDir*, std::string> handles;
int closes = 0;

using Pak = std::pair<int32_t, std::string>;

void Reset() {
  CHECK(handles.empty());
  tree.clear();
  opened.clear();
  closes = 0;
}

void CheckBalanced() {
  CHECK(handles.empty());
  CHECK_EQ(closes, int(opened.size()));
}

void RemappedTrilogy() {
  Reset();
  // Representative post-remap FST: MP2's files keep their original entry numbers,
  // while 9 menu + 20 Prime + 20 Corruption PAKs remain under root siblings.
  tree["/"] = {{1, true, "fE"}, {135, true, "Mp1"}, {585, true, "mP3"},
               {471, false, "RS5.PAK"}, {567, false, "Strings.pak"},
               {562, false, "SamGunFxMulti.PAK"}, {565, false, "SlideShow.PAK"},
               {569, true, "Video"}, {900, false, "opening.bnr"}};
  std::vector<Pak> expected{{471, "/RS5.PAK"}, {567, "/Strings.pak"},
                            {562, "/SamGunFxMulti.PAK"}, {565, "/SlideShow.PAK"}};
  for (const Pak& pak : std::vector<Pak>{{442, "AudioGrp.pak"}, {443, "FrontEnd.pak"},
      {444, "GGuiSys.pak"}, {445, "LogBook.pak"}, {446, "Metroid1.pak"},
      {447, "Metroid2.pak"}, {448, "Metroid3.pak"}, {449, "Metroid4.pak"},
      {450, "Metroid5.pak"}, {451, "Metroid6.pak"}, {452, "MidiData.pak"},
      {453, "MiscData.pak"}, {454, "NoARAM.pak"}, {560, "SamGunFx.pak"},
      {561, "SamGunFxLow.pak"}, {563, "SamusGun.pak"}, {564, "SamusGunLow.pak"},
      {568, "TestAnim.pak"}}) {
    tree["/"].push_back({uint32_t(pak.first), false, pak.second});
    expected.emplace_back(pak.first, "/" + pak.second);
  }
  for (const auto& [dir, count, first] :
       {std::tuple{"fE", 9, 2}, std::tuple{"Mp1", 20, 136}, std::tuple{"mP3", 20, 586}}) {
    for (int i = 0; i < count; ++i) {
      tree["/" + std::string(dir) + "/"].push_back({uint32_t(first + i), false, "Foreign" + std::to_string(i) + ".PAK"});
    }
  }
  tree["/Video/"] = {{570, false, "Background_Loop.thp"}};
  std::sort(expected.begin(), expected.end());
  CHECK(PortMods::detail::DiscPaks(true) == expected);
  CHECK_EQ(expected.size(), size_t(22));
  CHECK(std::find(opened.begin(), opened.end(), "/fE/") == opened.end());
  CHECK(std::find(opened.begin(), opened.end(), "/Mp1/") == opened.end());
  CHECK(std::find(opened.begin(), opened.end(), "/mP3/") == opened.end());
  CheckBalanced();

  opened.clear();
  closes = 0;
  const auto unfiltered = PortMods::detail::DiscPaks(false);
  CHECK_EQ(unfiltered.size(), size_t(71));
  for (const auto& pak : expected) {
    CHECK(std::find(unfiltered.begin(), unfiltered.end(), pak) != unfiltered.end());
  }
  CheckBalanced();
}

void OnlyRootDirectories() {
  Reset();
  tree["/"] = {{1, true, "assets"}, {2, true, "MP2"}, {3, true, "mp10"},
               {4, false, "fe.pak"}, {5, false, "mp1.PAK"}, {6, false, "mp3.pak"}};
  tree["/assets/"] = {{10, true, "fe"}, {11, true, "MP1"}, {12, true, "mp3"},
                       {13, true, "unreadable"}};
  tree["/assets/fe/"] = {{101, false, "a.pak"}};
  tree["/assets/MP1/"] = {{102, false, "b.PAK"}};
  tree["/assets/mp3/"] = {{103, false, "c.PaK"}};
  tree["/MP2/"] = {{104, false, "d.pak"}};
  tree["/mp10/"] = {{105, false, "e.pak"}, {106, false, "not.pak.txt"}};
  const std::vector<Pak> expected{{4, "/fe.pak"}, {5, "/mp1.PAK"}, {6, "/mp3.pak"},
      {101, "/assets/fe/a.pak"}, {102, "/assets/MP1/b.PAK"}, {103, "/assets/mp3/c.PaK"},
      {104, "/MP2/d.pak"}, {105, "/mp10/e.pak"}};
  CHECK(PortMods::detail::DiscPaks(true) == expected);
  CheckBalanced();
  opened.clear();
  closes = 0;
  CHECK(PortMods::detail::DiscPaks(false) == expected);
  CheckBalanced();
}
} // namespace

extern "C" int DVDOpenDir(const char* path, DVDDir* dir) {
  if (tree.find(path) == tree.end()) {
    return 0;
  }
  opened.emplace_back(path);
  handles[dir] = path;
  dir->location = 0;
  return 1;
}

extern "C" int DVDReadDir(DVDDir* dir, DVDDirEntry* entry) {
  const auto handle = handles.find(dir);
  REQUIRE(handle != handles.end());
  auto& entries = tree.at(handle->second);
  if (dir->location == entries.size()) {
    return 0;
  }
  auto& source = entries[dir->location++];
  *entry = {source.number, source.directory, source.name.data()};
  return 1;
}

extern "C" int DVDCloseDir(DVDDir* dir) {
  CHECK_EQ(handles.erase(dir), size_t(1));
  ++closes;
  return 1;
}

int main() {
  RemappedTrilogy();
  OnlyRootDirectories();
  return PORT_TEST_RESULT();
}
