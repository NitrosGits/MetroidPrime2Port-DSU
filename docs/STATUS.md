# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

## 2026-10-29: 0.6.2 released

- Bumped `versionCode` 8 / `versionName` "0.6.2" and added the 0.6.2 metainfo release entry
  (`cb344643e`); release commit `37a61d2c2` (code-identical; only the Flatpak manifest differs).
- Flatpak moved to runtime `org.freedesktop.Platform` 25.08, which closes the 0.6.1 end-of-life gap. The 25.08 SDK
  ships SDL3 3.2.30, which Aurora's auto provider picked and which lacks `SDL_SavePNG`, so the
  manifest now forces `-DAURORA_SDL3_PROVIDER=vendor` (`37a61d2c2`). The local build
  (12,246,864 B) installed once: `flatpak info` reports `Version: 0.6.2`, runtime 25.08,
  `files/` holds only the binary, metainfo, desktop file, icons and licences, and it reaches the
  disc prompt (run with this shell's `XAUTHORITY` set; without it, "x11 not available").
  Test install removed.
- CI on `37a61d2c2`, all `success`: linux-release 40852272521, windows 40852276725, macos
  40852280313, ios 40852284307, android 40852288057.
- Crash soak (binary-identical to `37a61d2c2`): `build/s-release-2026-10-29b/soak/summary.tsv`,
  **96/96 rows `ok`**, exit 0. Log `/tmp/opencode/soak-062.log`.
- Artefact checks from `build/release-assets-0.6.2/out` (12 assets): APK `versionCode='8'
  versionName='0.6.2'` signed by `5c380a13…8bd20`; IPA and macOS `Info.plist` 0.6.2; macOS
  executable `-rwxr-xr-x` in the zip; x86_64 binary needs at most `GLIBC_2.34` and its Build ID
  matches the `.debug`; no game data in any zip, APK, IPA, tarball or the x86_64 AppImage; no
  `user/` folder in the Windows zips; the new "Use twin-stick controller" string is in the
  Linux, AppImage and Windows binaries.
- APK installed on the test phone (`25102PCBEG`) as an in-place update: versionName 0.6.2 /
  versionCode 8.
- Tag `v0.6.2` = `37a61d2c2`; GitHub release
  https://github.com/Odrannnn/MetroidPrime2Port/releases/tag/v0.6.2, published as a **normal
  (latest) release**, not a pre-release, so the update check's `/releases/latest` finds it (it was
  a 404 with only pre-releases; now answers `v0.6.2`). 12 assets, all `uploaded`. Notes from
  `build/release-notes-0.6.2.md`, with no link to the deleted PrimeDecomp/echoes repo.

## Open items carried from older sessions

Sessions older than 0.6.2 moved to `docs/STATUS-history.md`; what is still open from them:

- **Android verification pending** (2026-10-26 widescreen/ball/suit batch): touch layouts at 16:9/21:9, the pause Inventory recolor and the ball shadow have not been run on the phone.
- **Shared spare copy buffer** (2026-10-20 scan-visor session): camera filters, `CPlayerGun` and the particles still copy into the shared spare buffer without a `Destination` and could take each other's resolved texture; fold them into `Destination` when one shows the symptom.
- **Echoes' prompt-glyph `kKeys`** (2026-10-19 twenty-fourth session): the game draws prompts from GUI models (`model_abutton`), not 32x32 textures; building `kKeys` needs a dump taken while a prompt is up plus the model's own texture size/hash.

Older sessions: `docs/STATUS-history.md`.


## Smoke rules (2026-10-24)

Poke your own family's actor; no `=> err` proofs; strictly increasing @f; objs checked >=40f after a kill; editor ids, not uids; no forced Dead. Beam-free lanes = `worker` + model `opencode-go/beam-free`; their reports are unreliable, so check con_*.out.
