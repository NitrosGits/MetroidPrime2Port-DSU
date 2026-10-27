// The data folder move's copy engine (port_data_folder.h): what it leaves out, that it replaces
// what is there, that a cancelled copy leaves nothing half-written, and the port_paths.txt rewrite.

#include "port_data_folder.h"
#include "port_test.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

void Write(const fs::path& path, const std::string& text) {
  fs::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << text;
}

std::string Read(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string(std::istreambuf_iterator< char >(file), {});
}

} // namespace

int main(int argc, char** argv) {
  const fs::path root = argc > 1 ? fs::path(argv[1]) : fs::temp_directory_path() / "port-data-folder-test";
  fs::remove_all(root);
  const fs::path from = root / "private";
  const fs::path to = root / "shared";

  CHECK_MSG(PortDataFolder::IsSkipped("port_settings.ini.partial"), "staging skipped");
  CHECK_MSG(PortDataFolder::IsSkipped("disc.iso.part"), "partial copy skipped");
  CHECK_MSG(PortDataFolder::IsSkipped("mods/a.CMDL.mpcopy"), "half-copied file skipped");
  CHECK_MSG(PortDataFolder::IsSkipped(".mpcopy_incomplete"), "incomplete flag skipped");
  CHECK_MSG(!PortDataFolder::IsSkipped("textures/a.png"), "a textures folder kept");
  CHECK_MSG(!PortDataFolder::IsSkipped("port_settings.ini"), "settings kept");

  CHECK_MSG(!PortDataFolder::HasData(from.string()), "a missing folder holds no data");
  Write(from / "a.png.part", "scratch");
  CHECK_MSG(!PortDataFolder::HasData(from.string()), "only skipped files is no data");

  Write(from / "port_settings.ini", "settings");
  Write(from / "USA" / "Card A" / "a.gci", std::string(3 << 20, 'c'));
  Write(from / "mods" / "remastered-models" / "1234.CMDL", "model");
  Write(from / "user_textures" / "textures" / "b.png", "pack");
  Write(to / "port_settings.ini", "old settings");

  const PortDataFolder::Totals totals = PortDataFolder::Measure(from.string());
  CHECK_MSG(totals.files == 4, "four files to move");
  CHECK_MSG(totals.bytes == 8 + (3 << 20) + 5 + 4, "their size");
  CHECK_MSG(PortDataFolder::HasData(to.string()), "a folder with settings holds data");

  PortDataFolder::Progress progress;
  std::string error;
  CHECK_MSG(PortDataFolder::CopyTree(from.string(), to.string(), progress, error), "copy succeeds");
  CHECK_MSG(progress.filesDone == 4 && progress.bytesDone == totals.bytes, "progress counts everything");
  CHECK_MSG(Read(to / "port_settings.ini") == "settings", "existing file replaced");
  CHECK_MSG(Read(to / "mods" / "remastered-models" / "1234.CMDL") == "model", "nested file copied");
  CHECK_MSG(fs::file_size(to / "USA" / "Card A" / "a.gci") == (3u << 20), "large file copied whole");
  CHECK_MSG(!fs::exists(to / "a.png.part"), "scratch not copied");

  const fs::path cancelled = root / "cancelled";
  PortDataFolder::Progress stop;
  stop.cancel = true;
  CHECK_MSG(!PortDataFolder::CopyTree(from.string(), cancelled.string(), stop, error), "cancelled copy fails");
  bool leftovers = false;
  if (fs::exists(cancelled)) {
    for (const auto& entry : fs::recursive_directory_iterator(cancelled)) {
      leftovers |= entry.is_regular_file() && entry.path().filename() != ".mpcopy_incomplete";
    }
  }
  CHECK_MSG(!leftovers, "cancelled copy leaves no files");
  CHECK_MSG(!PortDataFolder::IsComplete(cancelled.string()), "cancelled copy marked incomplete");
  CHECK_MSG(PortDataFolder::IsComplete(to.string()), "finished copy complete");
  CHECK_MSG(!PortDataFolder::HasData(cancelled.string()), "the incomplete flag is not data");

  // The pass before the switch copies only what changed since.
  Write(from / "port_settings.ini", "newer settings");
  PortDataFolder::Progress sync;
  CHECK_MSG(PortDataFolder::CopyTree(from.string(), to.string(), sync, error, true), "sync succeeds");
  CHECK_MSG(Read(to / "port_settings.ini") == "newer settings", "changed file re-copied");
  CHECK_MSG(sync.filesDone == 4 && sync.bytesDone == 14, "unchanged files skipped");
  CHECK_MSG(PortDataFolder::IsComplete(to.string()), "synced copy complete");

  Write(from / "keep.bin", "kept");
  PortDataFolder::DeleteData(from.string(), [](const std::string& rel) { return rel == "keep.bin"; });
  CHECK_MSG(Read(from / "keep.bin") == "kept", "kept file survives the delete");
  PortDataFolder::DeleteData(from.string());
  CHECK_MSG(!PortDataFolder::HasData(from.string()), "old copy deleted");
  CHECK_MSG(fs::exists(from / "a.png.part"), "scratch survives the delete");
  CHECK_MSG(!fs::exists(from / "mods"), "emptied folders removed");

  // port_paths.txt: the user= line is replaced, the others kept, and a missing file is created.
  const fs::path marker = root / "port_paths.txt";
  Write(marker, "user=/old\ncache=/c\ndisc=/d.iso\nenv.A=1\n");
  CHECK_MSG(PortDataFolder::WriteUserLine(marker.string(), "/new/"), "marker rewritten");
  CHECK_MSG(Read(marker) == "user=/new/\ncache=/c\ndisc=/d.iso\nenv.A=1\n", "only user= changed");
  fs::remove(marker);
  CHECK_MSG(PortDataFolder::WriteUserLine(marker.string(), "/x/"), "marker created");
  CHECK_MSG(Read(marker) == "user=/x/\n", "fresh marker holds the line");

  fs::remove_all(root);
  if (port_test::Failures() == 0) {
    std::printf("port_data_folder_tests: all passed\n");
  }
  return PORT_TEST_RESULT();
}
