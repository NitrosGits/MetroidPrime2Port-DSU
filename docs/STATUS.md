# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

## 2026-10-29: 0.6.1 released

- Bumped `versionCode` 7 / `versionName` "0.6.1" and added the 0.6.1 metainfo release entry;
  artefacts are built from that commit (`ad26e59e6`) by the release workflows.
- Tag `v0.6.1` = `ad26e59e6`; GitHub pre-release
  https://github.com/Odrannnn/MetroidPrime2Port/releases/tag/v0.6.1 with the twelve assets
  (`gh release view v0.6.1 --json assets` lists them all `uploaded`). CI: linux-release
  40791251657, windows 40791254843, macos 40791258032, ios 40791260907, android 40791264448,
  all `success` on `ad26e59e6`.
- Flatpak built locally from the same commit (12,058,096 B) and installed once for
  verification: `flatpak info` reports `Version: 0.6.1`, `files/` holds only the binary, the
  debug data and `share/{licenses,metainfo,applications,icons}`, and it starts and reaches the
  disc prompt. Test install removed afterwards.
- Crash soak on the release commit: `build/s-release-2026-10-29/soak/summary.tsv`, **96/96 rows
  `ok`**, exit 0 (mods/none × validate/release × MSAA 1/4 × the 12-area tour, 117 s per
  (mode, toggles) game). Log `/tmp/opencode/soak-061.log`.
- Artefact checks from `build/release-assets-0.6.1/out`: APK `versionCode='7'
  versionName='0.6.1'` signed by `5c380a13…8bd20`; IPA and macOS `Info.plist`
  `CFBundleShortVersionString 0.6.1`; macOS executable kept `-rwxr-xr-x` in the zip; x86_64
  binary needs at most `GLIBC_2.34`; no `.dol`/`.rel`/`.pak`/disc image in the tarballs, either
  AppImage, the macOS zip, the Flatpak or the APK (its `assets/` is licences plus textures). No
  version string exists in the Linux/Windows binaries, so the #4 fix was confirmed in them by
  the new `Scan Peek Resolve` label (4 hits in each; 0 in the 0.6.0 Windows exe), and their
  provenance is the CI runs above on `ad26e59e6`.
- Remaining known gap: the Flatpak runtime `org.freedesktop.Platform` 24.08 is now reported
  end-of-life by flatpak; moving the manifest to a supported runtime is the next release task.
- Release APK built locally with the project key (signer SHA-256 still
  `5c380a13...8bd20`, so it updates over 0.6.0 without uninstalling) and installed on the
  test phone (`25102PCBEG`, Android 16) as an in-place update: versionName 0.6.1 confirmed,
  save and settings kept.
- On-device check of issue #4: the game resumed in the scan visor with `msaa=4`, read-back
  logs showed six distinct EFB scan ids (28811/59536 non-zero pixels), and a logbook scan
  completed. Screenshots `/tmp/opencode/phone-0.6.1-launch.png` (mid-scan) and
  `phone-0.6.1-scan2.png` (after).

## Open items carried from older sessions

Sessions older than 0.6.1 moved to `docs/STATUS-history.md`; what is still open from them:

- **Android verification pending** (2026-10-26 widescreen/ball/suit batch): touch layouts at 16:9/21:9, the pause Inventory recolor and the ball shadow have not been run on the phone.
- **Shared spare copy buffer** (2026-10-20 scan-visor session): camera filters, `CPlayerGun` and the particles still copy into the shared spare buffer without a `Destination` and could take each other's resolved texture; fold them into `Destination` when one shows the symptom.
- **Echoes' prompt-glyph `kKeys`** (2026-10-19 twenty-fourth session): the game draws prompts from GUI models (`model_abutton`), not 32x32 textures; building `kKeys` needs a dump taken while a prompt is up plus the model's own texture size/hash.

Older sessions: `docs/STATUS-history.md`.


## Smoke rules (2026-10-24)

Poke your own family's actor; no `=> err` proofs; strictly increasing @f; objs checked >=40f after a kill; editor ids, not uids; no forced Dead. Beam-free lanes = `worker` + model `opencode-go/beam-free`; their reports are unreliable, so check con_*.out.
