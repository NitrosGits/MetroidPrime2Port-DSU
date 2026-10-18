# Session status (detail)

The Status section of AGENTS.md links here: this is the per-session detail, rewritten rather than
appended, dated as in the heading there.

## 2026-10-18 (thirteenth session): the phone becomes the target

The APK was built and installed (debug-key signed, because the installed copy is; the release key
would need an uninstall that deletes the app's private disc image and card). Everything below was
then driven by what the phone showed.

**Diagnostics** (`eea16f8a1`): the Android fault handler walks `_Unwind_Backtrace` and prints
`module+0xoffset (symbol)` frames, and `platform/port_breadcrumb.cpp` keeps a 48-entry ring of recent
actions (texture binds/frees, resources in no loaded pak) dumped as `[crumb]` lines before the
re-raise. It turned "SIGSEGV in resolve_static_texture" into "this 642x450 texture, this pointer" in
one run. Plus texture-safety fixes it exposed: `texture_source_is_mapped` checks every page of the
source, and `CTexture::UnloadBitmapData` re-loads the emptied object into the GX slots holding it.

**The morph transition** (`917f82173`): the crash was `CCameraFilterPass::DrawRandomStatic` passing
`((rand()+0x1f)&~0x1f)+0x8000` as texture data - low RAM on the console, garbage on the host. The
port owns a noise block now. The Morph Ball's missing inner glow was `CMorphBall::Render` never
drawing `mBallInnerGlowGen`; retail's table is plain white and its condition is "opaque and not
hacked". The later "lights up like a boost charge" report is retail's own pulsing glow (checked
against Dolphin) - the port's shell reads more golden, which makes it stand out.

**Colours** (`e32a7bc36`, `61336daf4`): Aurora reads every TLUT big-endian, and two palettes were
built on the CPU in host order - `CCubeRenderer::DrawScanVisor`'s scan lookup table (every scannable
object pink instead of blue) and the font palettes (`CTextRenderBuffer::GeneratePalette` reads disc
order; `CRasterFont::DrawString` builds its table in memory and needs the swap on the write side).
HUD text is white/light blue now.

**Scan visor**: the code was complete - `DrawScanEffect`, `CScanDisplay`, `CHudDecoInterfaceScan` all
match retail's calls, and `DrawDarkEffect`/`DrawEchoEffect` are empty in retail too. Scanning never
started because Scan Item reads the *analog* L trigger and the virtual pad only set the digital bit
(`251c2b04d`).

**Music** (`5a1361d56`): silent for three reasons - `SRSFHeader` read raw (magic `'SR'`, version
check failed, stream dropped), the stream pointer passed as MusyX's 32-bit `user` (a SIGSEGV on the
audio thread at the first refill), and MusyX reading the ADPCM predictor/scale from byte 3. Music
plays now (rms 1100-6600, MusyX peak 0 -> ~5000); its loop point is being fixed.

**The phone's F1 and touch controls** (`bed7f6ce7`, `7b88f433f`): the on-screen pad is MP1's
`TouchControlsView` copied, with all 48 natives implemented in `platform/port_touch_jni.cpp` (wheels,
Missile, L/R, START, the editor, aim, spring flick, hide/restore; minimap taps, pointer menus, the
toast, `touch:` bindings and wheel icons answer safely). The F1 window is MP1's full-screen page
window inside the SDL safe area with the Prime theme from `ui_common.cpp`, a page list, thumb-sized
rows at display scale clamped to 2, and `port_remap_ui`'s Controls > Remap page (click-to-arm
capture, per-row Default, Reset all, conflict markers). Still missing from menu parity: the search
bar, a UI scale control, the settings file, and video/audio pages with real backends.

**Aim and the Sanctuary crash** (`a35193272`, `0c33bd5a6`): a touch aim drag snapped the camera
back to level on release because `PortMouse::UpdateAim` treated touch as idle once `TouchAimActive()`
went false and synchronised to the free-look camera; MP1's direct aim is persistent relative deltas,
so a latch now keeps the aim the owner after the lift (-48.7 deg held at -48.6 after release on the
phone). And the Sanctuary door crash was not disc data: `FillInStaticGeometry` converts surface
headers and halfword sections in place, an occluded area's ARAM token stores the *converted* bytes,
and the return path converted them again - every material index became its byte-swapped self.
`CPostConstructed::mGeometryConverted` skips the second pass, and the model/resource parsers now
refuse data they cannot use with a one-line `[model]`/`[res]` message naming the asset instead of
dereferencing it.

**Settings file**: a lane started `PortSettings` (`<user>/settings.cfg`, env wins over the file) and
was cancelled mid-work; it compiled but a second run wrote the defaults back over the file
(`MP2_FAST_MORPH=0`), so it was reverted rather than committed. It needs a proper pass, including a
save only when a value actually changed.

**Not a bug**: the see-through Worker Splinter in Temple Grounds area 12 - its model (716998c9) is
additive with no depth write in the disc data itself (the game's creature log calls it a mucus
layer), unlike the ordinary Splinter's opaque model.

## 2026-10-18 (twelfth session): a log file, the automapper, and the F1 menu on the phone

Three things the phone asked for, all committed (`72d9e0d01`, `283278ddd`, `952ec24f0`).

**A log file.** `<user>/metroid_prime2_port.log` now gets everything the process prints - the
port's lines, Aurora's, SDL's, and what is printed just before an abort - with the previous run kept
as `metroid_prime2_port.old.log`; 64 MiB cap; `MP2_LOG_FILE=0` turns it off, F1 > Testing has the
row, `logfile [0|1]` the console command, and both say it takes effect next launch. On the desktop a
forked copier writes the pipe to the file and to the terminal; on Android the existing
stdout/stderr pump into logcat feeds it, and the file goes to the app's *external* folder
(`Android/data/org.metroidprime2.port/files`) because the private user dir cannot be read without
root - so `adb pull` works. Verified on the desktop (file created, rotation on the second run,
`MP2_LOG_FILE=0` writes nothing) and on the phone (`adb pull` gives a 229-line run log).
The `[aram]` staging lines are now behind `MP2_DEBUG_ARAM=1`: they were 253 of the 405 lines in a
30 s desktop run and thousands on the phone, enough to bury anything worth reading.

**The automapper.** The minimap drew as a fan of stretched lines over the whole view, and opening
the map screen crashed the phone in `aurora::gx::texture::resolve_static_texture`. Two causes:
`CMapArea::PostConstruct` converts the area's vertex floats to host order but `Draw` then used the
3-argument `CGX::SetArray` (the disc path: size 0, data marked big-endian), so Aurora uploaded
nothing or swapped it again - the vertex-array gotcha in AGENTS.md; the area vertices and the door
corners in `CMappableObject` now go through `SetArrayNative` with their real sizes. And
`CInGameGuiManager::DestroyAreaTextures`, then `CTexture::UnloadBitmapData` itself, freed texture
data while Aurora's FIFO thread was still resolving texture objects - `GXDrawDone()` before the
`CARAMToken` is dropped covers every free in one place. Verified on the desktop (minimap correct in
areas 0 and 5, map screen renders and closes, upstream text identical with TARGET_PC blocks
stripped) and on the phone (minimap correct, map screen no longer crashes).

**The F1 window on the phone.** It had no touch path at all, which is why the overlay carried no
MENU pill. Now it does: a MENU pill (in every layout, saved and movable like the rest) toggles the
window, and while it is open every touch except the pill is handed to SDL - whose touch-to-mouse
path is what ImGui reads, the way Metroid Prime 1's port does it - so coordinates and click timing
are SDL's own. Injecting ImGui events from Java was tried first and got both wrong. The pill is
hit-tested against the rect from its last draw, two fingers scroll the list (`nativeDebugWheel`),
and a drag on the window's empty space scrolls it too, which is why the window is no longer
movable. Verified on the phone: MENU opens and closes, a tap toggles the row under the finger
(Log file), a drag scrolls to the Saves section, and the map screen opens without the crash.

**The gray triangle** (`4c834b5ad`): not the map screen at all, though that is where it looked like
it came from - the Landing Site's fish swarm skins each boid on the CPU from the model's position and
normal arrays, and those stay in disc (big-endian) order in memory because the GPU's guest array path
swaps them itself. Read as host floats they are NaNs and values near 1e6, so every boid had garbage
vertices: a ball of strands on the desktop, one huge gray triangle on the phone's GPU (which
rasterises non-finite vertices lavapipe drops). That also explains the user's two clues - turning the
camera away cannot fix geometry that is rebuilt wrong every frame, and changing area drops the
swarm's model. `SwarmRenderHelpers.cpp` now swaps positions and normals, and
`CSkinnedModel::GetSkinnedPosition/GetSkinnedNormal` - what `CAnimData`'s bounds and
`CActorModelParticles`' emitters read - get the same swap, since they read the same arrays.
Verified on the phone: gameplay and the map screen are clean, the swarm draws as small white specks,
no crash in the run. Still open there: the Landing Site's radial "strand ball" is a separate effect
that has not been checked against retail, and the touch control set is still M3's (MP1's missile,
L/R buttons and triggers, beam/visor wheels, pointer menus for the game's own screens).

## 2026-10-18 (eleventh session): save tools (import/export/backup), and what of the slice does not fit

The fifth slice `docs/QOL.md` listed, taken as far as MP2's own types carry it: the card's save file
can now be read, exported, imported back and backed up before every write. MP1's other two pieces -
save states and quick resume - **do not fit this tree** and are named below rather than faked.

**The format, as it actually is.** `CMemoryCardSys::CCardFileInfo` is the authority (its read is
`StartRead`/`PumpCardRead`, its write `WriteFile`/`BuildSaveSlot`), and a save file is:

- 0x40 bytes: the GCI directory entry (game `G2ME`, maker `01`, name `MetroidPrime2`);
- 0x2000 bytes: the header block (comment, banner, icons, CRC) - irrelevant to the tools;
- two copies of `mAlignedUserDataSize` (0x2000 for this game), each a `uint` CRC then a `uint`
  generation (`BuildSaveSlot`), then 0x1ff8 user bytes;
- in the user bytes: `SSaveHeader` (signature, save index, three present flags, the `SAVH` marker -
  15 bytes), 192 bytes of system options, one 32-byte game-options block per slot, 32 bytes of
  global options (0x14f in all), then one `SGameFileSlot` (0xa38, starting with `GMST`) per present
  slot. `CMemoryCardDriver::ReadFinished` reads exactly this, and slot listing hands each buffer to
  the game's own `CGameState::LoadGameFileState`, so `save info` prints what the file select would.

**Two endiannesses, decided by the CRC.** The CRC and generation are written through a `uint*`
(`BuildSaveSlot`), so a save this port wrote has them **host-endian**, while one a console or Dolphin
wrote has them big-endian. `Inspect` computes the CRC over `block + 4` (the same call the writer
makes) and accepts the copy if the stored word matches either order; the order that matched is also
how the generation is read. The newer copy is then chosen by the greatest stored slot timestamp, not
by generation: the generations alternate 0/1 by construction, so the greater number is not the newer
copy (a save written during this session left generation 0 in one copy and 1 in the other, and the
1 was the new one only because the previous save happened to be 0).

**What is ported.** `platform/port_save.cpp` + `platform/include/port_save.h`:
`Directory`/`ImagePath`/`IsImage` (Aurora's `aurora_card_get_path`/`aurora_card_get_type`),
`Files` (the card folder's GCIs, by directory entry rather than file name), `Inspect`, `Export`,
`Import`, `Backup`/`Backups`/`BackupsDirectory`, and `BeforeCardWrite`. The console grows one
command with subcommands - `save [info] [<file>]`, `save export [<path>]`, `save import
<path|latest>`, `save backup`, `save backups` - and the setting is `MP2_CARD_BACKUP=1`, F1 > Saves
("Back up the card"), `cardbackup <0|1>` on the console, hooked at the top of
`CMemoryCardSys::CCardFileInfo::WriteFile` under `#ifdef TARGET_PC`, which is the one place both a
first create and every later write pass through. Backups, exports and replaced files live in
`<user>/save-backups` (`card-<stamp>.gci`, `export-<stamp>.gci`, `replaced-<stamp>.gci`); the newest
20 `card-*` copies are kept. An import never deletes: the file it replaces is copied to
`replaced-<stamp>.gci` first, and Aurora is remounted (`aurora_card_remount`) so the front end sees
the new file.

**Verified on the desktop** (`MP2_USER_PATH` a copy of the phone's card, `MP2_SKIP_FRONTEND=1`,
`MP2_CONSOLE=4780`, `tools/mp2con.py`):
- `save info` printed `copy 2 of 2 (generation 1)`, `slot 0: world 3bfa3eff, 0% items, 0% scans,
  00:00:02, 99 energy, 0 tanks`, matching the boot's own `[skip] card slots 0=3bfa3eff 0% 00:00`
  (two independent readers agreeing on the same file); `save info <old-backup>` printed copy 1 of 2,
  generation 0, 00:00:00.
- `save export` produced a byte-identical file (`cmp`), `save export <dir>/` kept the card's own
  name, and `save import latest` replaced the card file with a `replaced-*.gci` kept and the card
  unchanged afterwards (`cmp` after the round trip).
- `save backup` and `save backups` listed the copies; with 27 `card-*` files present a backup pruned
  to the newest 20 (oldest removed).
- The write hook: `MP2_INPUT="STATION@100,A@220+3,..."` with `MP2_CARD_BACKUP=1` fired
  `[save] card backup before the write to 'MetroidPrime2': .../card-20261018-093126.gci` on the save
  the save station's own A press made, and the card's copy 2 came back with generation 1 and a newer
  timestamp. Note that this A press leaves the run **paused in the save screen**, where no game
  update runs: the console stops answering (30 s, `[harness] no game update`), which is the same trap
  the sweep notes describe.
- Gates: `tools/gen_files_cmake.py --check` rc=0; the upstream matching build
  `Game Code: 94.14% fuzzy, 63.29% matched` rc=0; the only upstream file touched,
  `src/Kyoto/DolphinCMemoryCardSys.cpp`, strips to its `HEAD` non-PC text exactly (checked with a
  `#ifdef TARGET_PC` stripper, not by eye).

**What does not fit, and why (this is the part worth not inventing):**
- **Save states** (`port_savestate.cpp`, 1200 lines in MP1): a whole live world - actors, colliders,
  effects, pools - serialised and restored. MP2's `CGameState::PutTo` is the *save* state (world
  layers, player state, options), not the runtime; there is no equivalent serialiser to build on, so
  this stays the largest single item and is not started.
- **Quick resume** (MP1: a snapshot written on quit, loaded at launch): it is a savestate with a
  trigger, so it inherits the same wall. MP2's resume today is the card's own save, which
  `MP2_SKIP_FRONTEND=1` boots straight into (slot from the card header, or the first non-empty one) -
  that is the honest MP2 answer until savestates exist.
- **Region conversion** (MP1's `port_save_convert`): MP1 converts GM8E/GM8P because both releases
  share a save layout. MP2's save carries the world table's `SAVW` hashes and area/layer data that
  differ by release, so `Inspect` recognises `G2ME`/`G2MP` and *refuses* a foreign one with a clear
  error instead of converting it.
- **Raw card images**: Aurora may mount a raw image instead of a GCI folder. `Import` handles that
  (`aurora_card_raw_insert` + remount), but `Files`/`Inspect`/`Export`/`Backup` need paths, so on an
  image they report "the card has no save file". Neither this desktop nor the phone uses one.
- **Slot-level tools** (copy one save slot over another, erase one, rename a file): not done. It
  means rewriting the user data, both CRCs and the present flags offline; the front end already does
  it, and a port copy would have to reproduce its option-block shuffling. Next slice if wanted.
- **Dolphin-emulator and Trilogy-save folders**: MP1-specific discovery and formats; MP2's own save
  formats there are unknown, so nothing was guessed.

## 2026-10-18 (tenth session): gameplay tweaks

The fourth slice `docs/QOL.md` listed, and the first one with game-side hooks in *four* upstream
files: fast morph, spring ball, toggle lock-on and sticky charge. MP1 has all four, but its shape did
not transfer - MP1's morph transition is one duration scalar and its ball jump is a C-stick path in
`CMorphBall`; MP2's morph is `mMorphTime`/`mMorphDuration` set from four places in
`CPlayer::UpdateMorphBallState`, and its C-stick is read by the ball camera's aim code, not the ball.

**The settings.** Four statics in `platform/port_overlay.cpp` with `EnvFlag` reads for
`MP2_FAST_MORPH`, `MP2_SPRING_BALL`, `MP2_LOCK_ON_TOGGLE`, `MP2_STICKY_CHARGE`, getters/setters in
`platform/include/port_overlay.h` (plus `kFastMorphDuration` 0.2 s and `kStickyChargeAfter` 0.35 s),
one F1 > Gameplay row each with its env var and a one-line reason, and four console commands in
`CmdGod`'s shape - no argument toggles, an argument sets, so a headless run drives them one update
later. Nothing is saved; there is still no settings file.

**Fast morph** - `CPlayer::UpdateMorphBallState` computes `morphDuration`/`unmorphDuration` (1 s, or
0.6/0.5 s in multiplayer) and two screw-attack durations from `gpTweakBall`; the hook shortens the
first two when the option is on and leaves the screw attack's own transition alone, because those
are a different move with their own authored times. `mMorphTime` is also the morph factor the body
and camera interpolate on (`CPlayer.cpp`, lines ~3225 and ~3356), so the shorter duration shortens
the whole transition rather than cutting it off. Measured on the desktop by polling `status` in real
time around an `X` key press (a batch of console commands is *not* one command per update - the
console drains the queue within one update, which is what made the first attempts read nonsense):
morph 0.80 s off, under 0.1 s on, unmorph 0.80 s off and instantaneous on.

**Spring Ball** - `CMorphBall::ComputeBallMovement`, which the C-stick had no claim on: the ball
camera interpolates its own look-at and never reads `ARAUp`, and `PortMouse`'s twin-stick path is off
while morphed (`CPlayer::MouseAimAllowed`). The gate is `mBallState == kBS_Normal`, on the ground, not
morph-transitioning, movement allowed, not attached to an actor, Morph Ball Bombs owned, and 40
frames of cooldown (randomprime's number, in seconds). It calls the game's own
`CPlayer::BombJump(GetTranslation(), mgr)` and then does the two things MP1's version does around it:
it puts back the horizontal velocity `BombJump` zeroes (it is written for a bomb going off *under*
the ball, and a spring jump that stops the roll feels broken) and sets `kMS_FallingMorphed`, which is
what tells the physics the ball has left the ground on the tick it was still standing on it. Verified
by reading `status`'s z every 40 ms around `cstick 0 127 20`: -48.44 -> -45.04 and back with the
option on; not a millimetre with it off.

**Toggle lock-on and sticky charge** - MP1's two state machines, taken verbatim into
`platform/include/port_hold_toggle.h` (`Toggle` with its debounce and its "a latch that loses its
lock lets go", `Sticky` with its 0.35 s threshold and its swallowed release), with
`tests/port_hold_toggle.cpp` ported too and registered as `port_hold_toggle_tests` beside
`port_touch_pad_tests`. They are applied in `CStateManager::ProcessInput` to
`mFinalInputs[controller]` right after the pad has been read: the helper finds the player whose
`GetPlayerSelection()` is that controller (a controller index is not a player slot - `ProcessPlayerInput`
maps it the other way), and does nothing when input is disabled so a cutscene or menu resets both.
Two `PortSetL`/`PortSetA` setters in `CFinalInput` write the bits back; L sets the digital click *and*
the analog trigger, because MP2's orbit command can be bound to either and both are L on the pad.
Verified in area 11 (warped there with the console: area 0's Temple Grounds room has no `CPatterned`)
with two Dark Troopers in view. With the toggle on: holding L for 0.6 s gives `orbit 1` during the
hold and **both stay** after the key is up for as long as it was sampled (2.6 s); a tap then reads
`orbit 0, L=0` and does not re-lock; the next tap reads `orbit 1, L=1` again. With it off, L is a
plain hold (orbit 1 only while held). Sticky: a 0.6 s A hold then release leaves `A=1`; a tap
releases it (that is the charged shot) and the bit drops; with the option off the bit drops at
release.

**Review pass.** A fresh-context read of the commit found no functional bug and two soft spots,
both fixed: the state machines are shared, so a second controller that owns no player would have
reset pad 0's latch every tick (the helper now ignores a controller with no player), and the
latches were live through the scan display's soft pause (they now need `kGS_Running`). The second
one is deliberately narrow: the pause and map screens stop game updates altogether, so nothing can
run there and a latched L resumes with the game - the next press lets go. Spring Ball's side
effects (the velocity restore, `kMS_FallingMorphed`, the cooldown) would also apply on a `BombJump`
that returned early, but `mBombJumpState` is never set to `kBJS_BombJumpDisabled` in this tree, so
that branch is unreachable today.

**Instruments this slice needed.** `status` now prints `orbit N  game input L=x A=y` - the game's L/A,
not the pad's, which is the only way to see a latch from outside. The console's virtual pad could
only hold the main stick, so Spring Ball had no headless trigger: `PortInput::Hold` now takes both
axis pairs and `port_console.cpp` grew `cstick <x> <y> [updates]` beside `stick`. `tools/keytap.py`
did the rest - the morph button, which the console names `Y`/`X` for enter and exit, and the L/A
presses.

**Gates.** `gen_files_cmake.py --check` rc=0, `gen_console_names.py --check` rc=0, upstream matching
build `Game Code: 94.14% fuzzy, 63.29% matched` rc=0 (twice: after the lane's edits and after the
line-length pass). Each of the four upstream files was stripped of its `#ifdef TARGET_PC` blocks and
diffed against `git show HEAD:<path>` - all four identical, including the blank lines, which is why
the guarded blocks sit tight against the preceding brace rather than after a blank line.

**Not done / left open.** MP1's spring ball also fires on a gyro flick and a bound "spring ball"
action, and halves the jump inside a half pipe; none of that is here. The lock-on latch's
"target dies -> let go" branch is unit-tested only: it needs a kill, which the console cannot do
without a message id, and `kill` kills the player. Sticky charge is gated on owning Charge Beam
(`kIT_ChargeBeam`), which the test save has. The half-pipe interaction above is likewise unverified:
`kBS_Normal` and `OnGround` together looked unreachable there, but nobody drove a half pipe.

Re-verified after the review fixes on the same desktop run (area 11, a Dark Trooper in view): a
0.6 s L hold reads `orbit 1, L=1` and keeps both for the rest of a 2.4 s sample, a tap releases it,
and sticky A stays latched through 1.6 s of sampling with the beam visibly charging in a screenshot
(`/tmp/opencode/aim2.png`).

## 2026-10-18 (ninth session): keyboard bindings

The input slice `docs/QOL.md` listed third: rebinding over Aurora's pad key tables, with the F1
editor row, `bind`/`binds`, and one file under the user directory. MP1's `port_input_settings.cpp`
was the reference only for the shape of the editor - its settings model (profiles, toml, action
binding, touch layout) is far wider than this tree's, so the take is the rebinding part alone.

- **`platform/port_binds.{h,cpp}`** own the map. The table is 22 rows - the 12 pad buttons
  (A/B/X/Y/START/Z/L/R/UP/DOWN/LEFT/RIGHT) then the 10 stick inputs (`STICK_*`, `CSTICK_*`,
  `TRIGGER_L`, `TRIGGER_R`) - each with the port's built-in key, the pad input it drives and the
  action name the console and the file use. `Install()` replaces the literal `port_events.cpp` used
  to hold; `PADSetKeyButtonBindings`/`PADSetKeyAxisBindings` take the whole table on every change,
  because it is 22 tiny calls and tracking which row moved buys nothing.
- **`keybinds.cfg`** beside the memory card and `touch.cfg` (`MP2_USER_PATH`, else Aurora's
  prefer-path): `<ACTION>=<key>` per row, keys as SDL scancode names ("X", "Left Shift", any case)
  or `MOUSE_LEFT/MIDDLE/RIGHT/X1/X2`, `none` to unbind, `#` comments. Missing file = the built-in
  layout (deleting it is the reset), unknown rows and keys are skipped one log line each, and writes
  are atomic (tmp + rename). Read once on the game thread's first frame: the key names are SDL's, so
  it cannot be read in `main` before Aurora is up - `PortBinds::Initialize` only stores the path.
- **Aurora's own `keyboard_bindings.dat`** is neither written nor relied on. It is binary, has no
  names and Aurora loads it before the port's map replaces it; `keybinds.cfg` is the port's file.
- **F1 > Bindings** shows all 22 rows with their keys, two rows side by side (buttons left, stick
  inputs right, `SizingFixedFit` + `RowBg`). Clicking a row's key button arms it - the button reads
  "press a key..." with the active colour and the tooltip follows - and the next key-down the event
  pump sees is bound and saved. `PortBinds::OnKeyDown` returns true while a capture consumed the key,
  so the pump `continue`s past its hotkey switch: binding F5 does not warp the world on the way in.
  `Escape` cancels, a second click on the armed row cancels, closing the window disarms (`Draw`'s
  not-visible path), and "Reset to defaults" rewrites the file. A port hotkey still binds; the log,
  the console reply and the window all say the hotkey will fire with it.
- **Console**: `bind <action> <key>` (the key is the rest of the line, so `bind START Left Shift`
  needs no quotes), `bind reset`, and `binds` (the table plus the file's path). `Set` returns whether
  the file was written, so the reply distinguishes "saved" from "in memory only".
- **`tools/keytap.py`** is the instrument this slice needed: XTEST key presses on a display
  (`XAUTHORITY` unset, since a bare Xvfb needs none). `MP2_INPUT` and the console's `press` write the
  *virtual* pad, past the keyboard map, so nothing else in the tree can test a binding. A `--hold`
  of 20 ms was too short to be seen: SDL can process the press and the release in one frame's event
  queue, and the pad then reads the key as up. That looked exactly like "the binding did not work"
  and cost the first pass at the pause-screen test.
- **Verified on the desktop** (`MP2_TELEPORT=0`, area 0, `MP2_CONSOLE=4780`, the run driven from
  `tools/mp2con.py`, `tools/keytap.py` and an XTEST click helper; screenshots `/tmp/opencode/b*.png`
  and `c*.png`): `binds` lists the built-in layout and the path; `bind A g` saves `A=G` in the file
  and applies (`bind START Left Shift` proves the joined multi-word name, `bind A none` the unbound
  row, `bind reset` the whole table); `bind nosuchrow x`, `bind A nosuchkey` and bare `bind` answer
  the error text; the F1 row arms on a click, captures `A=G` with the tooltip following, Escape
  leaves `X=C` in place and Reset restores every row (the file's `STICK_UP=U` gone); `STICK_UP=u`
  walks forward and `W` then does not; after a restart `[binds] ...: 22 bindings` and the loaded
  `STICK_UP=U` still walks; `bind START p` then a real `p` opens the pause screen (the Inventory/
  Options/Data Network screen in `c4_pause_p.png`).
- A fresh-context review found no functional defect and three small ones, two of which went in: a
  UTF-8 BOM on `keybinds.cfg`'s first line is skipped (an editor writes one, and it otherwise lost
  exactly the first binding), and a key SDL cannot name (scancode 0: some media/Fn keys) is refused
  while the row stays armed, since it would be saved as `none` and be gone after a restart. The
  third - the F1 capture sees key-downs only, so a mouse button is bound from the console - is a
  hint in the window rather than code: a click that armed or pressed a row would otherwise also bind.
- Gates: `tools/gen_files_cmake.py --check` rc=0, `tools/gen_console_names.py --check` rc=0, the
  upstream matching build `Game Code: 94.14% fuzzy, 63.29% matched` rc=0. Every changed source is a
  platform file, so no upstream file changed and no `#ifdef TARGET_PC` block was needed.

## 2026-10-18 (eighth session): the rest of the graphics toggles

Three settings, the shapes `MP2_FOV` and `MP2_NO_HELMET` established: a field in
`platform/port_overlay.cpp` with its `MP2_*` env var and F1 row, a console command so a headless run
can drive the same thing, and one `#ifdef TARGET_PC` hook per upstream file.

- **Arm cannon offset** (`MP2_GUN_OFFSET="<right>,<forward>,<up>"`, `gunoffset <r> <f> <u>`, F1 >
  Render). Applied in `CPlayer::UpdateArmAndGunTransforms` where the game adds the tweak gun
  position: the user offset goes into `gunOffset` (so the shots leave from the moved muzzle) and into
  `grappleOffset` whenever the game is placing that arm itself. The morph-ball branch is untouched -
  it uses its own fixed offset. The overlay's three sliders plus "Reset arm cannon"; values are
  clamped to +-1 m and a non-finite one becomes 0, because the value lands in a transform.
- **MSAA and anisotropic filtering** (`MP2_MSAA=1|4`, `MP2_ANISO=1..16`, `msaa` / `aniso`, F1 >
  Render). Aurora took both only in `AuroraConfig` at init, so the vendored tree gained MP1's shape:
  `aurora_set_graphics_quality` queues `msaa << 16 | anisotropy`, `begin_frame` hands it to
  `webgpu::set_quality`, which synchronises, drops the bind-group caches (they hold samplers built for
  the old anisotropy) and rebuilds the framebuffers at the current size when the sample count changed.
  `PortOverlay::Draw` sends it only when the values changed, so an untouched port issues nothing.
  Anisotropy needs no rebuild: samplers are cached by their descriptor, which contains the anisotropy.
  The port's EFB read-back already reads the *resolve* target, so the scan visor and the camera
  filters survive 4x MSAA (checked with `visor scan`; `scan_peek` still reports the EFB as
  multisampled, which is its own older instrument, not the copy path the port uses).
- **HUD scale** (`MP2_HUD_SCALE=50..100`, `hudscale`, F1 > HUD). The whole HUD is scaled about the
  screen centre, rather than MP1's per-element "shrink toward the nearest edge", because MP2's GUI
  frame has no per-widget spread machinery to hang that on. `CSamusHud::Draw` sets a flag for the
  length of its own body (an RAII scope, so every early return clears it); `CGuiCamera::Draw` scales
  its orthographic extents - or widens the perspective FOV - by `100/percent` while it is set. The
  HUD frame, the radar and the scan interface all draw through a `CGuiCamera` inside that body, so
  they scale together; the helmet, memo screens, pause screen and inventory draw outside it and keep
  their size.

**Verified on the desktop** (`MP2_TELEPORT=0`, area 0; screenshots under `/tmp/opencode/qol_*.png`,
`v2_*.png`): the gun moves with `gunoffset 0.3 0.2 -0.1` and returns with `gunoffset 0 0 0`; the HUD
at 70% shrinks toward the middle with the helmet frame unchanged; `msaa 4` and `aniso 1` log
`Graphics quality: 4x MSAA, ... anisotropy` and the frame is unchanged apart from its edges; all four
from the env at start-up (`MP2_MSAA=4 MP2_ANISO=8 MP2_HUD_SCALE=80 MP2_GUN_OFFSET="0.25,0.2,-0.1"`)
apply before gameplay; the F1 rows read those values back; and `MP2_HUD_SCALE=60` with `MP2_INPUT=START@400`
draws the pause menu at the authored size. Gates: `gen_files_cmake.py --check` rc=0, matching build
`Game Code: 94.14% fuzzy, 63.29% matched` rc=0, and `CPlayer.cpp`, `CSamusHud.cpp` and
`CGuiCamera.cpp` strip to their pre-change non-PC text exactly.

## 2026-10-18 (seventh session, continued once more): hide helmet, hide visor effects

Two more graphics toggles, both env-only plus a checkbox in the overlay's HUD section:
`MP2_NO_HELMET=1` (the world drawn without the helmet frame) and `MP2_NO_VISOR_FX=1` (without what
`CPlayerVisor::Draw` paints for the active visor). The helmet is gated where the game already gates it,
`CInGameGuiManager::Draw`'s `mHelmetVisMode != 0 && notInCine` branch; its non-PC body is repeated
under `#else` rather than wrapped, because an added `&&` would change the matching build's bytes for
it. The visor hook is an early return after the light setup in `CPlayerVisor::Draw`, so the draws
after it keep the state they expect. Echo and dark visor effects are empty stubs in this tree, so the
second flag currently suppresses the scan visor's own frame and edges - the scan *window* is the
HUD's scan interface and stays, which is what a screenshot with both flags shows.

Verified on the desktop, each against the default frame: helmet gone with everything else intact
(`/tmp/opencode/hud_nohlm.png` against `fov_55.png`), and the scan visor with `MP2_NO_VISOR_FX`
(`/tmp/opencode/hud_nofx_scan.png`) keeps the scan window and the tint while the visor's own framing
is not drawn. Gates: `gen_files_cmake.py --check` rc=0, matching build still 94.14% / 63.29%, and
both upstream files strip to their pre-change non-PC text exactly.

## 2026-10-18 (seventh session, continued again): the first-person FOV setting

The first of `docs/QOL.md`'s graphics toggles: `MP2_FOV=<deg>` (F1 > Render > "Field of view (1st
person)", 0 keeps the game's 55). The first-person camera takes its FOV once, when it is built
(`CCameraManager.cpp:108` reads `mFirstPersonFov`), so a live change is applied in
`CCameraManager::UpdateCameras` (`#ifdef TARGET_PC`): set `mFirstPersonFov` (what a rebuilt camera
reads) and `CGameCamera::SetFovAndTarget` on the camera in use, skipping a cinematic camera, which
animates its own FOV from its spline. Verified on the desktop with `MP2_FOV=90`: `status` reports
`camera first-person fov 90.0` and the frame is visibly wider
(`/tmp/opencode/fov_55.png` against `/tmp/opencode/fov_90b.png`).

**A documentation error fixed on the way:** `docs/QOL.md` said the F1 overlay's settings live in
`platform/debug_ui.cpp` and are saved to `port_settings.ini`. They do not: `debug_ui.cpp` is in no
CMake target at all (it is Metroid-Prime-shaped, like `PortBoot.cpp`), so a setting added there
compiles nowhere and fails to link (which is exactly how this was found). The live overlay is
`platform/port_overlay.cpp`, and it keeps no settings file - every knob is an `MP2_*` env var.
Both files' docs are corrected.

## 2026-10-18 (seventh session, continued): `MP2_TELEPORT` at boot rendered nothing

**The white screen was a scripted camera filter left applying its flash.** A boot-time
`MP2_TELEPORT=<area>` landed the player correctly and the game ran (60 fps, 210 draw calls, 192k
vertices a frame) but the screen was a flat white or black - the HUD drawn over it, no world, no
gun. It was not area 5: the same hold appeared after warping back to area 0, and the same area 5
*reached from gameplay* rendered perfectly. The finding that named it: an `MP2_DEBUG_FILTERS=1`
instrument in `CCameraFilterPass::DrawFilter` printed an opaque white `kFT_Add` full-screen filter
every frame, and a gdb breakpoint on it led to
`CStateManager::DrawAdditionalFilters` -> `mCameraFilterPasses[player][0].Draw()`: one of the 11
per-player camera-filter passes, which only a level script drives (`CScriptCameraFilterKeyframe`,
`kSM_Increment` sets the filter over `mTimeIn`, `kSM_Decrement` takes it away over `mTimeOut`).
`send <keyframe> Decrement` to the area's white keyframe made the world appear instantly.

**Fix: the boot request waits for gameplay, which is the state a console `warp` runs in.** Served at
frame 0 - `mGameState` is already `kGS_Running` and `IsFullyInitialized()` already true during the
save's own load, so state alone cannot tell the two apart - the warp's hand-over to the game's own
transition left the destination's scripted flash applied. It is now held until
`PortHarness::GameUpdates() >= 120` (the point the harness itself calls "gameplay reached"), and
`[warp] serving a request at frame 120` says where it went. `PortWarp::PendingIsBoot()`
(`platform/include/port_warp.h`) is what the game asks; `MP2_INPUT` tokens and console warps are not
boot requests and keep their exact timing. Also on the arrival path, a warp now ends a
`kTT_Portal` transition the hand-over may have left running (it never fired in these runs; a warp is
a teleport, so no portal wipe has anything left to do).

**Verified:** `MP2_TELEPORT=5` at boot now renders the area (screenshot, `/tmp/opencode/dfr2.png`);
`MP2_TELEPORT=5` plus `MP2_INPUT=NEXTAREA@900+1` still hops to area 6 at frame 899 and area 6 renders
(`/tmp/opencode/sm1.png`); `MP2_TELEPORT=0` is unchanged. Instruments added while hunting, all
`#ifdef TARGET_PC` and off unless set: `MP2_DEBUG_VIS=1` (what `CStateManager::GetVisSetForArea`
decided, per area), `MP2_DEBUG_TRANS=1` (`CMFGame`'s flow state, transition phase and fade),
`MP2_DEBUG_FILTERS=1` (every full-screen camera filter with its colour). `status` grew the camera
(kind, FOV, position), the area's fog, the alive/visible area list with the current area's occlusion
state, the escape timer, the world transition type, the viewport, and the frame's draw calls and
vertices - each of those was a question this hunt had to answer.

Gates: `tools/gen_files_cmake.py --check` rc=0 and the upstream matching build still
`Game Code: 94.14% fuzzy, 63.29% matched`; the four upstream files touched
(`CStateManager.cpp`, `CMFGame.cpp`, `CCameraFilter.cpp`, and the earlier `CFilteredDelayAux.cpp`)
strip to their pre-change non-PC text exactly.

**The console's fourth wave** (`platform/port_console.cpp` only, no upstream file): `world <id>
[area]` is MP1's `warp <world>` under its own verb (this tree's `warp` is the area warp the sweeps
use, and renaming it would break them), `worlds` lists the memory card's world table - id, area
count, and the STRG asset holding the name - and `strg <id|name> [index|name]` reads a string table
by asset id or resource name. Two things `strg` needed: the token has to outlive the command (the
load is asynchronous, so a token that dies with it cancels it, and every call answers "still
loading"), and the wide strings are narrowed to ASCII for the socket. Verified on the desktop:
`worlds` (12 worlds, Temple Grounds current), `strg 0x7f47f635` -> "Sanctuary Fortress",
`strg STRG_AllPickupsFound_2` -> "100% of all pickups acquired!", and `world 1baa96c2` switching
Temple Grounds -> Sanctuary Fortress (67 areas, area 0 renders).

## 2026-10-18 (seventh session): the console's second wave, and a client that had never worked

**Seven commands, and the names they print come from the enums.** `platform/port_console.cpp` gains
`objs [filter]`, `obj <id>`, `send <id> <msg>`, `visor [name|0-3]`, `beam [name|0-3]`,
`tp <x y z>` and `items` (the list is in `kCommands`, the help block and `platform/include/
port_console.h`). Three details carried the work:

- **Class names are RTTI**, demangled with `abi::__cxa_demangle` (`typeid(ent).name()`), the same
  shape MetroidPrimePort's console uses - this tree has no per-class name table and does not need
  one. The scan prints `CScriptDoor`, `CPlayer`, `CScriptRelay`, ...
- **Message, state and item names are generated**, not typed: `tools/gen_console_names.py` writes
  `platform/console_names.inc` from `EScriptObjectMessage`/`EScriptObjectState` (CEntityInfo.hpp) and
  `EItemType` (CPlayerState.hpp), keeping only explicit initialisers, first name wins on a duplicate
  value (`kSS_APRC`/`kSS_Approach`), values at or above `0x80000000` are the negative ones and are
  skipped. `--check` is a gate like `gen_files_cmake.py --check`'s, so an upstream sync that adds a
  message shows up as a stale file rather than as a mislabelled connection.
- **Ids are hex, and both of the ids a session reads resolve.** `objs` prints `TUniqueId::value`
  (`u046e`) because `Value()` masks to 10 bits and two objects can share it; the argument is parsed
  as hex (with or without `u`/`0x`), and `obj`/`send` fall back to the *unique* object whose
  `Value()` matches, so the id in a port log line (`[door] 006e got kSM_Open`) can be typed straight
  in. Ambiguous matches are refused by name.

**Visor and beam switching go through the game's own paths.** `visor` calls `StartTransitionToVisor`
and then the holster/draw the game does after a switch (`CPlayer::UpdateVisorState`,
`CPlayer.cpp:1330`); `beam` calls `CPlayerState::SetCurrentBeam`, which `CPlayerGun::HandleWeaponChange`
reads on the next update (`CPlayerGun.cpp:2555`) - the same path the `kIT_SwitchWeapon*` items take.
Neither forces a visor or beam the save does not own: it answers `=> err: ... not owned`, because a
switched-to beam the gun will not draw is a confusing state, not a useful one.

**`tools/mp2con.py` hung on every command in its default mode, before this session.** It read until
the terminator *and* kept reading because the `break` sat inside `if quiet:`; the port keeps the
connection open until the client hangs up (one client at a time), so a non-quiet run always ended in
`TimeoutError`. It now counts replies (`=> ok`/`=> err:` lines) and stops when the last command has
answered, and reports a bare timeout as "no reply from 127.0.0.1:<port>" instead of a traceback. The
console's own commit (`0e267e186`) was verified with `nc`, which has the same "wait for EOF" shape
and hid this.

**Verified on the desktop**, `MP2_CONSOLE=4780`, `MP2_TELEPORT=0` (area 0), `MP2_GODMODE=1`:
`objs door` (7 doors, class names demangled), `objs player`, `obj u046e` / `obj 046e` / `obj 0x46e`
(one door, three spellings) and `obj 400` -> `u0400 CPlayer hp 99/99`, `send u046e open` ->
`[door] 006e got kSM_Open` (and its partner door `01b5`), `visor scan` -> screenshot with the scan
tint, the scan window and the gun holstered, `visor combat` -> screenshot with the gun back, `tp`
8 m with the facing kept (screenshot plus `status`), `items` (11 lines), and the refusals:
`beam light`/`visor echo` not owned, `send ... NosuchMessage`, `obj 9999`.

**One thing found on the way, not caused by the console:** a `MP2_TELEPORT=5` boot lands the player
in area 5 at the area's first spawn point and renders *nothing* - white screen, the HUD's map markers
only, no energy bar and no gun - while `[ids]` counts and `status` are normal (`235 objects`, health
99). `MP2_TELEPORT=0` in area 0 renders correctly from the same card, and a fresh area-5 boot that
never touched the console shows the same white frame, so the console is not involved. Next session's
first look: whether the area's geometry paks are loaded after that warp at all (`MP2_DEBUG_AREA=1`,
and the `[resloader]` warnings).

**The console's third wave: the pad, the inventory, and a bow towards the camera.** `press <a+b+...>
[updates]` and `stick <x> <y> [updates]` hold buttons and the main stick on the same virtual pad
`MP2_INPUT` writes (`platform/port_events.cpp` grew `PortInput::Hold`, in `port_input.h`; the hold
merges into the script's state, so the two can be held at once, and a run with neither leaves pad 0
to the real controller exactly as before). `give <item> [n]` / `take <item> [n]` take `items`' own
names and go through CScriptPickup's path (`AddPowerUp` + `IncrPickUp`, so an energy tank heals and a
suit upgrade drops), `face <yaw deg>` turns the player through `CPlayer::Teleport` (0 = +y, 90 = +x),
and `screenshot` asks Aurora for the F12 capture and says where the log names it. Verified:
`stick 0 127 120` walked the player 5.6 m (position before and after), `press x 30` entered the morph
ball and the next one left it, `press a 40` fired, and — the regression that matters — an
`MP2_INPUT=DOOR@900+1` script still fired its token while console holds were in use, so the two
share the virtual pad without disturbing the sweep's driver.

**`give` + `visor` found a real crash on the first try: leaving the echo visor ended the run.**
`CPlayer::UpdateEchoVisorEffects` (`CPlayer.cpp:4459`) adds a filtered-delay aux effect while the
visor is up and removes it when it goes down; the removal reached
`ShutdownFilteredDelayAux` -> `CMemory::Free` on buffers that `rs_new` allocated, and on the host
`rs_new` is *the host's* `new[]` (`CMemory.hpp`'s TARGET_PC `#define rs_new new`), i.e. malloc
memory - so the game allocator walked back from a malloc block into its own metadata. ASan named it
in one run: "heap-buffer-overflow, READ of size 8, 56 bytes before 19840-byte region", allocated by
`PrepareFilteredDelayAux` and freed by `CMemory::Free`. Fixed in `CFilteredDelayAux.cpp` with the
port's own idiom for this (`#ifdef TARGET_PC` -> `::operator delete[]`, whose `PortDelete`
(``PortMwccNew.cpp`) routes by address - the shape `CFrameDelayedKiller` and `CDvdFile` already use).
Verified: echo -> combat -> echo -> combat with no fault in the normal build, and the ASan build
reports nothing at all in that sequence.

**Gates:** `tools/gen_console_names.py --check` rc=0, `tools/gen_files_cmake.py --check` rc=0, and the
upstream matching build still `Game Code: 94.14% fuzzy, 63.29% matched` - no `src/` or `include/`
file was touched this session, so the non-PC text is untouched by construction.

## 2026-10-17 (sixth session): the scan visor's desktop crash placed and fixed; the intro text skipped

**The scan visor's device loss was a Z8L depth copy resolved through the colour blit.** Entering the
visor lost the GPU two frames in (`VK_ERROR_DEVICE_LOST`, GPUVM at `0x80011ca000`, `SQC (data)`), and
every earlier attempt to place it failed because it is not in the visor's own drawing:
`CStateManager::CapturePlayerTextures` copies the 32x64 scan-target region three times - the id
buffer as `GX_CTF_A8`, then the same region as `GX_TF_Z16` and `GX_CTF_Z8L` for the depth buffers.
Aurora has depth conversion pipelines for Z8/Z16 but none for **Z8L**, so with a scaled destination
("the port scales copy destinations to the render target") that copy fell into
`tex_copy_conv::blit` - a *colour* passthrough pipeline - with the **depth** view bound to it. An
invalid descriptor, a garbage texture fetch, a GPUVM fault. `RADV_DEBUG=hang` agreed: the failing
draw is a one-sample passthrough fragment shader, and `MP2_NO_BLIT=1` (a knob since removed) made the
run play to its frame limit, which placed it. It is visor-specific only because those copies exist
only there.
- **Fixed on both sides.** The port no longer issues the two depth copies (their only product was the
  player's CPU depth buffers, and `PortScanTarget::FillBuffers` already fills those from the same
  read-back as the id buffer; the textures they resolved into were never sampled), and Aurora refuses
  to route a depth source through the colour blit - it warns once and leaves the target unresolved.
- **Also fixed in passing:** `gfx::find_bind_group` checked its lookup with `CHECK`, which compiles to
  nothing under NDEBUG, so a cache miss returned `end()->second` - a garbage bind group handed to the
  GPU. A lookup now refreshes the entry's frame and a miss logs and returns an empty bind group.
- **Verified:** `MP2_INPUT=LEFT@200+2` enters the visor, the run plays to its frame limit and exits 0,
  and the visor draws correctly - tinted world, scan window with its reticle, HUD
  (`/tmp/opencode/visor_desktop3.png`, the first desktop capture of the scan visor, matching the
  phone). Commit `875a1fa82`.

**MP2_SKIP_INTRO now skips the level-load text too.** A skip-intro run still showed the opening text
cards - "--- Data Confirmation ---", "Last transmission received from a rogue planet ... Aether",
"Mission File 02546" - typed over black, and *that* was what the run waited for: the first game update
at 33.9 s, gameplay at 36.0 s. A new `MP2_DEBUG_IOWIN=1` names the windows on `CIOWinManager`'s draw
stack once a second, which placed the cards in `CMFGameLoader::Draw` -> `CWorldTransManager`: the
*level-load text* `CMFGameLoader` enables (`STRG_IntroLevelLoad`) when the save has no powerups and
the world is the default one - a `kTT_Text` transition, so the loader blocks until it ends.
`PortHarness::IntroSkipLevelText` (true while `MP2_SKIP_INTRO` is armed) stops the loader enabling
either that text or the world-name card an area load shows; the transition still runs and
`UpdateDisabled` finishes it in two seconds, which is what lets the game start.
- **Verified on the desktop, same save:** with `MP2_SKIP_INTRO=1` there are no cards at all, `the
  first game update is at 2.7 s`, `gameplay reached at 4.7 s (frame 121)` (was 33.9 / 36.0 s) and the
  area plays (`/tmp/opencode/u_10.png`); with `SKIP_INTRO=0` the cards are back unchanged
  (`/tmp/opencode/v_10.png`). Commit `876f43f40`. With the cinematic skip and the memo dismissal that
  were already there, a skip-intro run now lands in gameplay with nothing of the intro left.

**Gates this session:** `tools/gen_files_cmake.py --check` clean, and the upstream matching build green
after each of this session's port edits in upstream files (`Game Code: 94.14% fuzzy, 63.29% matched`,
`rc=0`), all of them inside `#ifdef TARGET_PC` with the non-PC text byte-identical.

**A skipped intro cinematic now ends its scene** (`0201ba38e`). Stopping the cinematic camera and
handing over control is not the same end state as the scene playing out: whatever the sequence drives
keeps going under the first-person view. An `MP2_SKIP_INTRO` request now takes the debug setting's
strong path (`sHarnessCinematicSkip`): the scene's own skip first, and when a sequence has no skip
function, the flow *stays* in `kFS_CinematicSkip` - where it already runs at max speed without
drawing - so the sequence finishes by itself instead of being cut off. Camera-only is now the last
resort after 30 s of flow time. Phone: one skip request, `[skip] cinematic 0123: the scene's own
skip`, gameplay at 4.8 s, no second cinematic camera (the previous build re-requested camera 00f4 -
it had handed over control mid-sequence). Desktop unchanged (`the scene's own skip`, 4.8 s).

**Fixed: shooting a door into another room crashed the phone build** (`71f296b71`). The cause was an
**in-place `memcpy` in `CGameArea::DecompressAreaData`**: an uncompressed MREA chunk is expanded where
its source already sits, with the two ranges overlapping by a few hundred bytes (source ahead of
destination). That is undefined for `memcpy`, and which direction a libc copies in decides whether the
area's section data survives - glibc's x86-64 copy goes the safe way, which is why the desktop only
ever showed the *symptom*, a surface pointer in `CCubeModel`'s constructor that is not in the area's
buffers, while the phone (bionic) died. AddressSanitizer on the desktop replay named it: 11
`memcpy-param-overlap` reports, every one in that function, e.g. destination `[0x…5280,0x…9280)`
against source `[0x…5434,0x…9434)`; `memmove` for the port, `memcpy` unchanged for the console (`#else`).
After it: the ASAN build reports nothing on the whole journey, and the phone ran the full route three
times with no crash.
- **The replay recipe, on the desktop** (`USERDIR=/tmp/opencode/mp2card/user`, the same card the phone
  has - the two are byte-identical): `MP2_INPUT=DOOR@300+1,A@320+150,SUP@500+300,DOOR@1000+1,A@1020+400`
  - break the door in front of you, walk through into the neighbouring area, warp to *its* door and
  break that, which is what rebuilds the original area's geometry.
- **If it ever bites again:** `MP2_GUARD_GEOMETRY=1` swaps a surface pointer that is not in the area's
  mapping for the model's first surface (a wrong-looking model instead of a dead process), and
  `MP2_DEBUG_AREA=1` prints what it found.

**Previously (instrument, now explained): shooting a door into another room crashes the phone build.** `CCubeModel::CCubeModel` reads a
surface pointer that is not in the area's buffers while `CWorld::PropogateAreaChain` adds the newly
visible area's static geometry (fault `0x72c1753804`, surface `0x72c17537f8`, every section the fill
logged at `0xb4...`). Reproduced on the phone in ~35 s with `MP2_INPUT=DOOR@300+1,A@320+400` from the
save; the instrument is `MP2_DEBUG_AREA=1` (`CGameArea::AddStaticGeometry`/`FillInStaticGeometry` and
`CCubeRenderer::AddStaticGeometry` log the area, the ARAM transfer result, whether the models were
constructed, and the section/surface pointers, naming an out-of-buffer one), plus `MP2_DEBUG_DOORS=1`
and `MP2_WARP_DOOR=<id>` for `DOOR@frame` (commit `41a55178d`). The crash log names the failing fill:
the *original* area's second `FillInStaticGeometry` (`aram=1 constructed=0 instances=1`).
- **Ruled out: the in-place re-conversion.** That path converts the surface sections in place on every
  fill (`CCubeSurface::ConvertSurfaceHeader` and the surface-group halfword swap), so a second fill of
  already-converted buffers would double-swap them - but the second fill read the *same* surface count
  as the first (1858), so the buffers had been re-read from ARAM and were fresh. No double conversion.
- **The lead now:** every section pointer the fill captured sat in the area's own mapping
  (`0xb4...`), and the `CCubeModel` constructor is reached only after the whole fill returned and
  `CCubeRenderer::AddStaticGeometry` had run `CCubeModel::MakeTexturesFromMats` over the area's
  material section. So the bad entry (a pointer whose high half is not the area's) appeared
  *between* capture and use, inside that window - heap corruption from the material/texture step, or
  a write past the end of the surfaces vector. Next: log the instance's entries *inside*
  `AddStaticGeometry` (index and value of the first out-of-mapping entry, before the constructor runs)
  to tell "captured bad" from "corrupted afterwards".

**The phone, with both fixes (`tools/android_apk.sh :app:assembleRelease -PandroidNodStub=false`).**
`the first game update is at 2.7 s`, `gameplay reached at 7.0 s` (was 34.0 s / 36.0 s), and the run
plays (`/tmp/opencode/phone_skipint.png`, `ph_2.png`). Also on that build: the **flickering
camera-attached triangle** the person described is still there - one screenshot (22:23) shows a flat
grey wedge over the left half of the viewport with a hard diagonal edge, two seconds later it is gone.
It is neither the cutscene geometry (`457c86fe2`) nor anything this session touched, and the person's
own later read is that it may not be the cutscene at all; it stays the one unexplained rendering
artifact in ordinary phone gameplay.

**Next.**
- The person's scannable-highlight check on the new APK.
- The other CPU-bytes consumers the EFB copy can now serve cheaply: `CFluidPlaneCPU` (reads the
  half-size RGB565 copy as data), `CCubeRenderer::GetRenderToTexBuffer`'s reflections, and the
  mip-chain path in `GenerateScreenMipmaps`.
- M4, the release APK polish.

## 2026-10-17 (fifth session): the EFB-into-CPU copy - the scan visor's spare buffer

**What it is.** The console's `GXCopyTex(dest, clear)` copies a region of the framebuffer into main
memory at `dest`, in the format the preceding `GXSetTexCopyDst` chose. Aurora resolves the same copy
into a *texture*, which serves every consumer that samples it as a texture, but the game also treats
some of those destinations as bytes: the scan visor uploads the spare buffer as a C8 texture whose
texels are palette indices, the camera filters upload it as a half-size RGB565 backdrop, and the scan
window's pane does the same at its own size. The port now mirrors those copies for the CPU:
- `PortEfbCopy::Request` (new, `src/MetroidPrime/PortEfbCopy.cpp`) is called immediately before the
  `GXCopyTex` it mirrors. It issues `GXRequestEfbCpuCopy`, a FIFO command that takes the copy's
  geometry, destination size and texel format from the `GXSetTexCopySrc/Dst` state that is current at
  that point, so a call site cannot disagree with the copy.
- `aurora::gfx::efb_copy` (`extern/aurora/lib/gfx/efb_copy.cpp`) is the read-back, in the shape of
  `scan_peek`: the request rides on the pass the copy closes, the pass's *resolve target* (the same
  pixels and format the texture path produced) is copied to a mappable buffer at encode time, mapped
  in `after_submit`, and converted on the CPU to the copy's texel format - A8/R8/G8/B8/I8 as one
  byte, RGB565 big-endian, which is what the game's upload of console data expects. One to three
  frames of latency; a seqlock hands the bytes to the game thread.
- `PortEfbCopy::Fill` is called from both `CGraphics::LoadDolphinSpareTexture` overloads, just before
  the upload, and copies the latest completed read-back for that buffer and size into it. Nothing is
  written from the render thread, so an upload never sees half a copy.
- Three call sites, one mechanism: `CCubeRenderer::DrawScanVisor` (A8, viewport - the visor's palette
  indices), `CCameraBlurPass::GetFbCopy` (RGB565 half, or I8 full - the camera filters) and
  `CPlayerVisor::DrawScanEffect` (RGB565 - the scan window). The results screen reads the same buffer
  and is covered by the same fill once a matching copy has been requested at its size.
- Knobs: `MP2_EFB_COPY=0` turns the whole thing off (the A/B), `MP2_DEBUG_EFB_COPY=1` logs the fill
  rate, and `MP2_NO_COPY_TEX=1` keeps a copy out of Aurora's copy-texture map so the game's own
  upload of the CPU bytes is what the GPU samples - which is how the desktop verification isolates
  the new path.

**Verified.** Desktop, camera filter, end to end: with `MP2_NO_COPY_TEX=1` the pause screen's
backdrop is a blurred frame with the copy on (`[efb-copy] first spare-buffer copy landed (320x224,
143360 bytes, sequence 2)`) and **black** with `MP2_EFB_COPY=0` - `q1_2.png` against `q2_2.png` in
`/tmp/opencode`. Gameplay and the same pause screen with the copy-texture path left on are unchanged
(`r1_a.png`, `r2_a.png`). Phone (release build installed, `MP2_INPUT=LEFT@600+2`): the visor opens,
`[efb-copy] 120 fills from 357 requests (640x448 into ...)` shows the A8 viewport copy landing every
frame, the frame rate is 60 fps in the visor (300 game frames per 5.00 s, the same as before the
visor), and the A/B with `MP2_EFB_COPY=0` shows the broken overlay - a large flat olive wedge over the
scene (`phone_off.png`) where the copy on gives a coherent tinted view (`phone_v3.png`).
**Not verified:** scanning an actual scannable and reading its highlight colour on the phone - the
save's view here has nothing scannable in the scan window (`[scan] no target: every one of the 2048
id pixels is zero`), and the desktop cannot show the visor at all (below). That check is the person's.

**Correcting the record on the visor's desktop device loss.** It is **not** fixed by `5e3e122ee`
(zeroing the spare buffer) - it was fixed in the sixth session, at the top of this file: a Z8L depth
copy resolved through the colour blit. Reproduced today, three times, with `LEFT@200+2`/`LEFT@600+2` in areas 0
and 2: `VK_ERROR_DEVICE_LOST`, GPUVM fault at the same address `0x80011ca000`, `SQC (data)`, every
time - and it still happens with the visor's overlay draw skipped, `MP2_SCAN_PEEK=0`,
`MP2_NO_COPY_TEX=1` and `MP2_EFB_COPY=0` all at once, so it is in the visor's own rendering and not
in any EFB-copy path. `RADV_DEBUG=hang` puts the fault in a plain one-sample fragment shader (a blit),
i.e. a bind group whose texture or view is already gone; the address is stable across runs and is not
in the driver's address-binding report. The earlier "it plays to its frame limit" reading came from
runs with `input=0 bytes` - they never pressed LEFT, so they never entered the visor. The same goes
for the "flat white viewport" screenshot (`visor4_a.png`): that run had no input either, so the white
is the save-load fade, not the visor. The real desktop symptom is the crash; the real phone symptom
(the wrong highlight colours) is what this session's copy fixed.

**Gates.** `tools/gen_files_cmake.py --check` clean (the new port source is in `PORT_ONLY`, so
`files.cmake` gained one line); the upstream matching build is green after this session's four port
edits in upstream files - `Game Code: 94.14% fuzzy, 63.29% matched`, `rc=0` - and every one of them is
inside `#ifdef TARGET_PC` with the `#else` text byte-identical to the revision before the session
(`DolphinCGraphics.cpp` keeps its non-PC `GXInitTexObj`/`GXInitTexObjCI` calls verbatim in an `#else`
branch for exactly that reason).

**Next.**
- The visor's desktop device loss (above) - the one thing standing between the desktop and a
  screenshot of the scan visor.
- The phone check the person can do in a minute: scan a scannable and see whether its highlight is a
  single colour from the palette rather than garbage.
- The other CPU-bytes consumers the same mechanism can now serve cheaply: `CFluidPlaneCPU` (reads the
  half-size RGB565 copy as data), `CCubeRenderer::GetRenderToTexBuffer`'s reflections, and the
  mip-chain path in `GenerateScreenMipmaps`, which are still on the texture path.

## Current state as of 2026-10-17 (moved out of `AGENTS.md`, verbatim)

## Status (2026-10-17, fourth session: M2/M3 verified, three crashes/bugs fixed, PC-first loop)

Detail, including the repro commands for every open wall, is in `docs/STATUS.md`.

- Gates green: `tools/gen_files_cmake.py --check`, and the upstream matching build after this session's
  one port edit in an upstream file (`Game Code: 94.14% fuzzy, 63.29% matched`, `rc=0`). Commits:
  `befa10ead` (sweep tools), `c600728c6` (ball camera fix), `6f7d192ea` (`B` presses, RELOAD verdict).
- `tools/sweep_areas.sh` + `tools/sweep_report.py` walk all 63 Temple Grounds areas in 4 shards in ~10
  minutes and print a per-area verdict table. Press `B`, never `START`, never `A` (see Gotchas).
- Fixed and verified twice: the handed-hop crash in `CBallCamera::FindDesiredPosition` (a dangling
  reference in `CCameraManager::GetLastCameraTransform()` that GCC folded to null). `MP2_WARP_HANDED=1`
  now forces the handed route, which makes that path a 90-second reproduction.
- Open, worst first: (1) ~~areas 14, 21 and 35 stop updating~~ - **resolved `05efe2ecc`: the player was
  dying** in the dark-world areas and the run then sat on the lose-game movie's "continue" screen
  (`docs/STATUS.md` wall 1 has the backtraces); the sweep heals twice per area now, has a `DIED`
  verdict, and a shard over areas 12..23 is clean; the sub-question is whether `A` picks "continue"
  there, which would make the RELOAD verdict's "walks into a save station" story wrong; (2) area 53's
  hop never lands in a long run - **resolved `a5b08f7ae`**: the destination was stuck in
  `kP_WaitForValidation` because a dock in area 52 preloads 53 and holds its validation until a door
  the warp never opens, and `PortWarpSettle` now releases that hold. Reaching area 53 also turns out
  to *be the ending* (the credits; `TELEPORT=53` is that, not a stall), so the report still owes a
  `CREDITS` verdict; (3) ~~an `MP2_INPUT` token before the game's
  first update is deferred and never served~~ - **resolved `4401a70e2`**: a token's frame is a game
  update now (`platform/README.md`, "Input"), so frame 0/1 is the state the game's first update reads
  and a token whose frame has gone by fires at the first frame the script sees; the counter still
  advances through a pause, so a script can press `B` to dismiss one.
- Unverified as before: hazards as gameplay, the logbook, escape/endgame, lock-on handoff, morph ball
  and gyro input, the empty-`optional_object` audit.
- **The swarm vertex explosion is fixed** (`23c928626`): the flying bugs of the Temple Grounds crater (and every other swarm and the fish cloud) drew through the guest array path with host-skinned arrays. The user's clue - "caused by the tiny flying bugs" - is what pointed at `CFlyerSwarm` and `CSwarmBasics`; the nest's web beside them is a real object.
- This session, on the phone: **M2 and M3 verified** (the SAF picker copies a picked ISO to `files/disc.iso`
  and the next launch boots with no picker; the strict first-update proof passes; touch alone walks and
  fires). **A crash fixed**: morphing then unmorphing SIGSEGV'd because `CSamusFaceReflection`'s
  destructor is `inline` in another TU and the port filled the missing symbol with a logging link stub,
  which left the face model's resources locked; defined in the class's own TU now (`bc4c733c3`), and the
  same sequence leaves the process running. **`MP2_SKIP_INTRO=1`** (`75874aff5`): with
  `MP2_SKIP_FRONTEND=1 MP2_SKIP_CINEMATICS=1 MP2_TELEPORT=<area>` a run reaches a controllable player
  with no input - 36.0 s on the phone, 39.1 s on the desktop, against ~3 minutes of hand-pressed `A`
  before. `MP2_SKIP_FRONTEND` still gives up on an empty card, and the 34 s before the first update is
  loop-bound ARAM staging, not phone I/O (both in `docs/STATUS.md`).
- **The HUD-over-cutscene and the wedge beside it are fixed** (`457c86fe2`): the port's cinematic skip
  stopped the camera but not the scene, so the intro played on under the first-person camera with the
  HUD and visor over it - the wedge was cutscene geometry, not a broken model. **The swarm boids no
  longer explode** either (`23c928626` + `651988088` + `95d6dc3ca`), and a `TARGET_PC` guard that had
  leaked out of its block (matching build 63.29% -> 63.25%) is back. Iterate on the desktop build;
  the phone is for the final APK check (`docs/STATUS.md` has the run and shot commands).
- Open, and the reason the `MP2_DEBUG_DRAW`/bounds instrumentation exists: first-person world geometry
  that renders as a vertex explosion (dark maroon slabs and white
  strands; `/tmp/opencode/ve_fp3.png` and `m2.png` are phone frames of it). Not a reach stub (`0 of 45`
  are referenced), not a missing resource, and the face reflection only draws on a visor transition. The
  phone reproduces it on every run in that area.
- The Android APK the user asked for: plan and the M0-M3 detail in `docs/ANDROID.md` (section 4).
  **M0 done**: `tools/android_probe.sh` cross-builds this tree for arm64-v8a into
  `build/android-aarch64/libmetroid_prime2_port.so` - `AArch64`, exports `SDL_main`/`aurora_main`,
  `llvm-nm -u` all platform symbols, offline (Dawn/SDL/Rust caches in `build/android-deps`). The
  REL arena gate passed **on the user's phone** (32 MiB arena at `0x12000000`, the console's
  `0x81000000` base honoured exactly, no `MAP_32BIT`; fallback is a `/proc/self/maps` gap scan,
  covered on Linux by `port_arena_placement_tests`). **M1 done**: `android/` + `tools/android_apk.sh`
  build and install `org.metroidprime2.port`, which mounts the user's own disc through the real
  Rust nod on the phone (86 modules initialised, card loaded, renderer up, game loop running); fd
  1/2 are mirrored to logcat and the dirs resolve from `files/port_paths.txt`. Two known Android
  issues are recorded in `docs/ANDROID.md`: a one-off Dawn Vulkan-surface SIGABRT when launched
  with the screen locked, and the debug variant's `-O0` musyx inline link error.
  **M2 verified on the phone**: the `env.NAME=value` bridge in `files/port_paths.txt` (the `set`
  lines, `paths chosen` and `disc mounted` are in `docs/ANDROID.md`), the SAF picker with its
  G2ME01-header check and `.part`-then-rename copy (picked from `/sdcard`, `copying the disc
  image, 100%`, `disc image picked: content://…, copied to …/files/disc.iso`), and a relaunch
  after that boots with **no picker**. The strict `[harness]`/`[ids]` "first update" proof passed
  too: `the first game update is at 33.9 s`, `[ids] frame 0: 352/1024 ids allocated`.
  **M3 done and device-verified** (this session): `platform/port_touch.{cpp,h}` + MP1's
  `platform/touch_pad.{h,cpp}` + `tests/touch_pad.cpp` + `android/.../TouchControlsView.java`
  (the SDL virtual pad, a third aim source in `PortMouse`, the settings in `<user>/touch.cfg`, the
  JNI bridge, and a phone-only "Edit" pill for the layout editor). On the phone a run with
  nothing but touch walks the player (`[touch] axis 1 -1.00`) and fires (`[touch] button 0
  down/up`), the pad attaches as `Metroid Prime touch gamepad`, and the editor opens, switches
  GameCube/twin layout and saves it. `port_touch_pad_tests` covers the pad on the host;
  `MP2_DEBUG_PAD=1` prints one line per touch event for a device log. Next: **M4**, the release
  APK polish.

---

Gates: `tools/gen_files_cmake.py --check` clean, and the upstream matching build green after this
session's port edit in an upstream file (`Game Code: 94.14% fuzzy, 63.29% matched`, `rc=0`). Commits:
`befa10ead` (the sweep tools), `c600728c6` (the ball camera fix), `6f7d192ea` (`B` presses, the
RELOAD verdict).

**Still open, and how to settle each**
- **A flickering triangle from the viewport's lower-left, camera-attached** (the person's words), seen
  before the cinematic fix. That fix explains one wedge - cutscene geometry under the first-person
  camera - so this may be the same thing. It is **unconfirmed**: the person has not yet played the
  build with `457c86fe2` on the phone. If it persists in ordinary gameplay it is a second bug; the
  cheapest reproduction is the desktop crater run (`USERDIR=/tmp/opencode/mp2card/user TELEPORT=0
  tools/run_gameplay.sh`), not the phone.
- **The packed-UV lead is closed, as an instrument bug rather than a game bug** (`6b33c3336`): the
  materials whose TEX0 indices only fit the packed array are the ones flagged for it, and the draws
  that looked wrong are the world-shadow pass's black material, which returns before any array is
  chosen and whose texgen reads positions only. The scan now checks each surface against the array its
  own material selects.
- **The areas 14/21/35 update stall** (below) is being worked with `CIOWinManager`'s win list as the
  instrument, which is the thing to read at the stall rather than the functions that stop being called.

**Fixed this session, all verified on the desktop build** (rendering, cinematics and HUD questions
reproduce there, so they are not worth driving through the phone):
- **The HUD drawn over the landing cutscene, and the large translucent wedge beside it, are one bug**
  (`457c86fe2`). The port's cinematic skip stopped the cinematic *camera* (`StopCinematics`) and left
  the *scene* playing, so the intro ran on under the first-person camera with the HUD, the visor and
  the faceplate over it - the wedge is cutscene geometry seen from the wrong camera, not a broken
  model. Retail's skip (`CStateManager::SpecialSkipCinematic`) also fires the scene's skip
  connections, so it jumps to its end; that is what the port calls now, with `StopCinematics` as the
  fallback for a scene with no skip function. Controlled A/B: with `SKIP_INTRO=0` the cinematic has
  no HUD at all, with `SKIP_INTRO=1` it had the whole HUD over it, and with the fix `SKIP_INTRO=1`
  skips the intro outright and reaches the crater at the same time (33.9 s / 35.9 s).
- **A port edit that leaked out of its `TARGET_PC` guard** (`95d6dc3ca`). The swarm residency guard
  added a helper and an `if` in `CSwarmBasics.cpp` without the guard, and the upstream matching build
  dropped from `63.29%` to `63.25%` matched - less than one file, which is exactly why the number is
  worth reading after every upstream edit rather than glancing at "files OK". Guarded, and checked by
  stripping `TARGET_PC` blocks from all 18 files this session touched and diffing against the
  pre-session revision: identical everywhere.

**The scan visor** (the person's report: "you can't lock onto things to scan"):
- **The lock-on's missing piece is fixed** (`e8de72e7b`): MP2 picks its scan target from a 32x64 region
  whose ids live in the framebuffer's destination alpha, copied into CPU buffers and histogrammed.
  The copy went through the console `GXCopyTex` path while Aurora resolves that into a *texture*, so
  the buffers were never written. Measured first: the ids *do* reach the EFB alpha (`31/2048 pixels
  non-zero, palette 4` in area 2), only the copy-out was missing. `scan_peek` mirrors Aurora's
  `depth_peek`; one to three frames of latency, which the consumer does not mind.
  **Still unproven end to end**: the `[scan] palette N selected object X` line has not been seen,
  because on the desktop the visor could not be entered at all (below) and on the phone the build with
  the fix has not been tried yet.
- **The spare buffer is zeroed at allocation** (`5e3e122ee`): entering the visor lost the device
  (GPUVM, `SQC (data)`), and the uninitialised spare buffer was the suspect, since the visor binds it
  as a C8 texture and samples its bytes as palette indices through a 248-entry TLUT; zeroing is what
  the console's heap had and keeps the worst case inside the palette. **The crash is not fixed,
  though** - see the fifth-session section at the top: it reproduces with every EFB-copy path off,
  and the verification runs behind that commit never entered the visor.
- **The shared feature - an EFB-into-CPU read-back at viewport size to fill the spare buffer - is
  in** (fifth session, top of this file): the scan visor's overlay (`CCubeRenderer::DrawScanVisor`),
  the camera filters (`CCameraFilter.cpp:569-676`) and the scan window all read it through
  `PortEfbCopy`, with the same encoder-task shape and a frame of latency.
- **Confirmed on the phone (the person's report)**: scanning works - objects now lock on and scan -
  but **the highlight colours are wrong**. That is this same missing spare-buffer copy: the visor
  colourises by sampling it, and on the phone it holds uninitialised host memory, so the palette
  lookup gives random colours. `e8de72e7b` fixed the *targeting* (which reads the separate 32x64
  region), and `5e3e122ee` stopped the desktop dying on the same buffer; neither fills it.
- **Correcting an earlier note**: `MP2_INPUT` D-pad tokens *do* switch the visor (`LEFT@200+20`
  works). What looked like the press failing was the visor opening onto its broken overlay - a white
  viewport with the HUD intact, which is index 0 of the palette after the zeroing above. The
  `MP2_DEBUG_SCAN` selection line staying silent is a separate question about that diagnostic's call
  site, not evidence the visor never opened.

**Open, and it explains why the scan visor's bug survived so long: entering the scan visor kills the**
**desktop build about two frames in** with a GPUVM fault (`VK_ERROR_DEVICE_LOST`, `SQC (data)`). The
read-back lane hit it while trying to prove a scan target gets selected, and reproduced it with the
**pre-change** binary, with its own peek disabled, with the scan-window copy disabled, and in areas
0/1/2/3 - so it is in the scan visor's own rendering and predates that work. On the phone the visor
renders, which is why the person could see the visor at all. Worth fixing first, because it is the
reason desktop verification of anything scan-related was impossible: the swapchain/device loss also
means a scan-visor run cannot be screenshotted. **Still open as of the fifth session** (top of this
file), which ruled out every EFB-copy path as the cause and left a blit sampling a destroyed bind
group.

**This session (2026-10-17, fourth).** Verified on the user's phone rather than reasoned about:
- **M2's two open checks pass.** The SAF picker copies a picked ISO to `files/disc.iso` (`disc image
  picked: content://... , copied to .../files/disc.iso`), the next launch boots with **no picker**, and
  the strict first-update proof prints `[harness] the first game update is at 33.9 s` / `[ids] frame 0`.
- **M3's touch controls work with nothing but touch**: a held press on the drawn A pill advances the
  game's own boot screen, the floating stick walks the player (`[touch] axis 1 -1.00`), A fires, and the
  Edit pill opens the layout editor, switches GameCube/twin stick and saves it.
- **Morphing and unmorphing no longer crash** (`bc4c733c3`). The face reflection's destructor is
  `inline` in another TU, so no symbol was emitted and the port filled it with a logging link stub; the
  stub left the face model's resources locked and the second reflection (built on unmorph) died with
  SIGSEGV. Measured before: `[reach-stub 0001] _ZN20CSamusFaceReflectionD1Ev` then signal 11.
- **The swarm boids no longer render as exploded geometry** (`23c928626`) - the maroon curtains and
  pale slabs that covered the Temple Grounds crater's view. Cause: `CSwarmDisplayList::Draw` bound the
  swarm's host-skinned position/normal arrays through the *guest* array path (size 0, big-endian), so
  Aurora uploaded nothing or byte-swapped floats. One fix covers `CSwarmBasics`, `CMetareeSwarm`,
  `CPlantScarabSwarm`, `CIngBlobSwarm` and `CFishCloud`, which all draw through that class. The white
  radial structure left on the wall is the bugs' nest, not a bug - the user confirmed it.

**Regression sweep after this session's fixes (63 areas, 4 shards, `TAG=postfix`).** 48 of the 53
areas the shards reached are `OK`, **including 14, 21 and 35** - the old "stop updating" wall, which was
the player dying, now plays 3000/1200/900 updates and reports `OK`, so the death diagnosis and the
second `HEAL` fixed it and area 35 (unverified when that fix landed) is confirmed. **No crash and no
reach stub in any area**, so the nine fixes this session changed nothing for the worse. Two things the
table leaves open:
- **Area 46 ends in `STALL`**, and the log's last line before the harness report is
  `[warning] [aurora::gpu] Device lost: Device was destroyed` - four shards were running software
  Vulkan in parallel, so this reads as the renderer losing its device under that load rather than a
  game stall. Worth one confirmation run on its own (single shard, `AREA=46`) before it is called
  either way.
- **Area 53 is still the hop that never lands** (`asked for but never arrived: [53]`), the same gap as
  before; shard 3 played to its frame limit, so this is the long-standing wall and not a new one.

**The sweep exists and walks the whole world cheaply.** `tools/sweep_areas.sh` boots the save's own
world, teleports each shard to its first area and walks its slice with `NEXTAREA` tokens, all through
`tools/run_gameplay.sh`; `tools/sweep_report.py` turns the shard logs into a per-area table (verdict,
updates played, id high-water, missing resources, hop route, reach stubs) plus a coverage block from
the sweep's manifest. Four shards, all 63 areas, ~10 minutes of wall clock. Press `B`, never `START`
and never `A` - the gotcha above says what each does to a run.

**Verified this session**
- **The handed-hop crash is fixed** (`c600728c6`). `CCameraManager::GetLastCameraTransform()` returned
  a reference into a temporary that had already died; GCC folded it to a null pointer and
  `CBallCamera::FindDesiredPosition` read the transform through address 0. Verified twice on Temple
  Grounds 10 -> 11 with 588 ids free, the handed route: the natural replay of the original sequence and
  a forced one (`MP2_WARP_HANDED=1`, added this session - it makes this path a 90-second reproduction
  instead of ten hops of accumulated core). Both arrive, spawn and run on; before the fix both died.
- 29 areas played clean in a single pass (0..14, 16..21, 32..35, 48..52): hops land, the post-arrival
  drop leaves 600-900 ids free, the id count stays flat, no missing resources, no crash.

**Walls the sweep found, worst first**
1. ~~**Areas 14, 21 and 35 stop updating.**~~ **Resolved (`05efe2ecc`): the player was dying.**
   A dead single player takes retail's game-over path - `CMFGame::EndGame` sets `kRM_EndMovie2`,
   `CMFGame` leaves the `CIOWinManager` stack, and `CMainFlow` puts `CPlayMovie(kWM_LoseGame)` on it,
   a "continue from last save" screen that takes no `B` - so the run sat there and the harness's
   "no game update for 30 s" was a symptom, not a hang. Measured under gdb with a backtrace per
   `AddIOWin`/`RemoveIOWin` and the two window lists side by side: in area 21,
   `[player] killed in area 21 at update 1194 by weapon type 17 - the dark-world atmosphere ...`.
   Areas 21 and 35 carry the dark flag (as do 13 and 28), and one `HEAL` per area left ~23 s between
   heals against a death ~19 s after one. The sweep now heals twice per area, the report has a `DIED`
   verdict that outranks `STALL` and `RELOAD`, and a shard over areas 12..23 exits 0 with every area
   OK. **Open sub-question**: does `A` pick "continue" on that screen? If it does, the `AGENTS.md`
   Gotcha's "`A` walks into a save station" explanation for the RELOAD verdict is wrong rather than
   incomplete. Also unverified: area 35 with the second heal.
2. ~~**Area 53's hop never lands**~~ **Resolved, and it was two separate things (`a5b08f7ae`).**
   - **The hop**: not a stall and not a death. A per-300-frame trace put the destination in `phase 9`,
     `kP_WaitForValidation` (`CGameArea.cpp:1312`), which only advances when `mValidationPaused` clears.
     A script in area 52 sends `kSM_Load` to the dock towards 53 (`CScriptDock.cpp:177`), loading it
     ahead of time and holding its validation until the door opens - and a warp never opens the door.
     The isolated hop arrived only because it left 52 before that script ran. `PortWarpSettle` now
     clears the hold while it streams (`[warp] area N: its validation was held by a dock; releasing
     it`), and the same run logs the release, `moved to area 53`, its spawn and `[area] 53`.
   - **The "moves itself to 56 and freezes" is the end credits**: `TELEPORT=53` reaches the ending -
     `[area] 56`, ids down to 60, then no updates, with "METROID PRIME 2 ECHOES STAFF CREDITS" on
     screen (`CCredits`, reached through `CScriptSpecialFunction::AcceptCredits`,
     `CScriptSpecialFunction.cpp:1066`). Nothing is wrong; a sweep walking into 53 is watching the
     credits. **Named now**: the port prints `[end] the credits are rolling` as `CCredits` goes on the
     window stack, `tools/sweep_report.py` has a `CREDITS` verdict for that phase (it ranks below
     `DIED`, above `RELOAD`/`STALL`, and its coverage line says the slice ends at the ending), and
     `tools/sweep_areas.sh` documents the slot list including the ending.
3. ~~**An `MP2_INPUT` token that fires before the game's first update is deferred and never served.**~~
   **Resolved: a token's frame is a game update now.** The script counted `PortPumpEvents` iterations,
   so every token under the boot's ~2040 iterations landed in the boot: a warp token went to
   `PortWarp`'s deferred slot while the boot request was pending and the *next* token overwrote it
   (which is why `NEXTAREA@1400` never hopped where `NEXTAREA@3500` did), and a button's hold was over
   before the game had read a pad. Frame `N` is now game update `N` - `PortHarness::GameUpdates`, the
   same axis as `MP2_FRAMES` - frame 0/1 is the pad state the game's first update reads, and a token
   whose frame has gone by fires at the first frame the script sees instead of being dropped
   (`platform/README.md`, "Input", has the table). Verified: `MP2_INPUT="HEAL@1"` and `"HEAL@0"` both
   log `[input] HEAL at frame 1 (0.0 s)` and the game serves them *in* its first update (`[warp]
   healed` prints before `[harness] the first game update is at 34.0 s`, which the same update emits
   after the warp tick); `HEAL@2500,B@3000+4` fires on updates 2500 and 3000, which the `[ids] frame
   N` lines interleave exactly; and a 3-area sweep (`tools/sweep_areas.sh`, `DWELL=400`, build-token)
   hops 0 -> 1 -> 2 at `NEXTAREA@400`/`@800` with every token served, exit 0, `sweep_report.py` all
   areas OK. Two properties worth keeping in mind: the counter still ticks while the game is *not*
   updating (a HUD memo pauses it, and the sweep's periodic `B` is what dismisses that), and it stays
   at 1 before the first update, so a screen that precedes every game update - the front end - cannot
   be driven by frame at all: `MP2_SKIP_FRONTEND` is the way past it. Warp tokens also log where they
   fired now (`[input] NEXTAREA at frame 400`), so a log can tell a token the game ignored from one
   that never fired.

**`MP2_SKIP_INTRO=1`** drives a run into gameplay from the port side, with MP1's `MP_SMOKE_WORLD`
semantics: boot the save, serve the boot warp (`MP2_TELEPORT`/`MP2_WORLD`), wait for it to land, report.
No input is synthesised. Before gameplay it (a) answers `CMFGame::EnterMessageScreen` (the HUD-memo
pause) with the game's own dismissal, `UnpauseGame` + the HUD-message frame counter, never pausing - a
real pause menu does not go through it; (b) gives a current cinematic camera the skip request START
makes (`MP2_SKIP_CINEMATICS=1` makes unseen ones skippable; without it the save's area-0 opening stays
in a cinematic camera for 900+ updates). "Gameplay reached" = 120 consecutive updates running, in
`kFS_InGame`, no warp pending/streaming/handed over, no cinematic camera; logged once as `[harness]
gameplay reached at <s> (frame <update>)`. Budget `MP2_SKIP_INTRO_FRAMES` (default 3600 game updates),
then `gave up`. `MP2_SKIP_FRONTEND` now keeps removing "SplashScreen" windows until the chain has been
quiet for 30 iterations. Desktop, `u_a10`'s card: area 5 reached at 39.1 s, area 0 at 36.0 s; the card
shows no memo, so (a) was only exercised with an injected memo (dismissed, game kept updating).
Area 0's opening visor boot-up ("SCANNING") is still on screen at 53 s, but stick input turns the
player then (the minimap rotates).
**Gap:** an empty card still plays the front end's file select (`MP2_SKIP_FRONTEND` gives up on an
empty slot instead of starting a new game).

**The ~34 s before the first game update is now the largest delay in every run**, and it is the same on
the desktop (33.8-34.0 s, five runs) and the phone (33.9 s), so it is not phone disc I/O: it is
loop-bound - the loop runs at ~60 Hz and ARAM staging moves 64 KiB per iteration (the `[aram] ...
fileSize2=` lines step by 65536).

**Still unverified**: hazards as gameplay, the scan logbook's persistence, escape/endgame paths,
lock-on handoff, morph ball and gyro input; and the empty-`optional_object` read audit
(`CGunWeapon::IsChargeAnimOver` first).

**Android APK** (asked for 2026-10-17, same session). **M0 and M1 done**; detail and evidence in
`docs/ANDROID.md` (section 4, the "M0 detail" and "M1 detail" subsections). In short: M0 is
`tools/android_probe.sh`, which cross-builds arm64 into
`build/android-aarch64/libmetroid_prime2_port.so` (`AArch64`/`DYN`, exporting
`SDL_main`/`aurora_main`, every undefined symbol a platform one) with the bionic fixes in
`platform/main.cpp` (no `execinfo`; logcat + tombstones; explicit `<aurora/main.h>`) and
`platform/rel.cpp` (arena fallback = a `/proc/self/maps` gap scan below 4 GiB), and the arena gate
measured **on the phone** (32 MiB at `0x12000000`, `0x81000000` honoured exactly, no `MAP_32BIT`).
M1 is `android/` + `tools/android_apk.sh`: `org.metroidprime2.port` installs, loads that library
with the **real Rust nod**, mounts the user's own disc on the phone, initialises all 86 modules,
loads the card, brings the renderer up and runs the loop; fd 1/2 are mirrored to logcat and the
directories come from `files/port_paths.txt`. New tests: `port_arena_placement_tests`
(`-DMP_ARENA_FORCE_PORTABLE` runs the Android arena path on Linux). Known Android issues, both in
the doc: a one-off Dawn Vulkan-surface SIGABRT when launched with the screen locked, and the debug
variant's `-O0` musyx inline link error (worked around in `android/app/build.gradle`). Unproven on
device: the front end visually (the phone was locked) and the strict "first update" - the harness
env bridge is now in place for it (`env.NAME=value` in `port_paths.txt`), and it is proven working
on the phone, but the run itself needs the phone foregrounded, awake and untouched for a couple of
minutes. **M2 implemented**: the SAF disc picker (`EnsureDiscImage` in `platform/main.cpp`) - a
`G2ME01` header check, `SDL_ShowFileDialogWithProperties`, then a `.part`-and-rename copy into
`<files>/disc.iso`; it runs before `aurora_initialize` because the system picker would otherwise
destroy the swapchain (this Aurora snapshot has no `aurora_release_lost_surface`). Its device
verification is the one still-open item. Next: **M3**, the touch controls.

**Next steps, in order**: (1) the update stall, with the repro above and the two candidates named;
(2) the 53 -> 56 freeze if it survives (1); (3) the hazards and the layer areas of `1baa96c2` area 11
as gameplay; (4) the family bodies in the priority order; (5) a no-present option.

