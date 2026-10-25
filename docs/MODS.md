# Mods folder

Ported from MP1's first mods slice (MP1 `05cbf722a`): loose-file and PAK overrides plus a Mods list in
F1 > Game. Texture-pack pipelines, suit colours and downloads are not part of it.

`mods` in the user path (`MP2_USER_PATH`, created on first start; `MP2_MODS` points elsewhere) holds one
folder per mod, applied in name order so a later name wins; folders starting with `.` are skipped.
Inside a mod:

- a file at a disc path (`Metroid5.pak`, `Audio/...`, `Video/...`; case-insensitive) replaces that disc
  file. Only existing disc files can be replaced; others are reported and ignored.
- a file named `<8 hex digits>.<TYPE>` (`1A2B3C4D.TXTR`, `0552A456.STRG`), anywhere in the mod, replaces
  that resource, uncompressed, in every PAK holding it. The PAK is served as a virtual file whose table
  points past the original data, where the loose file is appended (32-byte aligned).
- `.txt/.md/.json/.png/...` files are ignored silently.

`platform/port_mods.cpp` (scan, plan, register with Aurora's DVD overlays) runs from `main.cpp` right
after the disc mounts; `platform/port_mods_pak.cpp` is the unit-tested PAK table / virtual file / reader
half (`tests/port_mods.cpp`, ctest `port_mods_tests`). PAK format is shared with Prime 1 (version
0x00030005, 20-byte table entries), so the parser is unchanged. Log lines start with `mods:` on stderr.
F1 > Game > Mods lists the mods with a switch each (`mods_disabled`) and the problems found; changes
apply on restart. An absent or empty folder registers no overlay at all.
