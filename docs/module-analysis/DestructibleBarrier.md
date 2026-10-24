# DestructibleBarrier REL (module 13): analysis notes

CScriptDestructibleBarrier (DBAR): a wall built from a grid of chunks (CBarrierChunkGrid). Hits destroy
chunks (ApplyDamage/DamageChunk, OnChunkDestroyed bursts the four particle effects), disconnected top
layers split off as sections (SplitOffTop; with a GRNT connection a generated actor stands in for each
section), the collision is rebuilt as an OBB tree (BuildCollision/BuildNode/BuildRowNode/AddBoxGeometry),
and after enough damage the barrier lowers into the ground and can be raised again (Think, Reset).
Source: upstream `src/MetroidPrime/ScriptObjects/CScriptDestructibleBarrier.cpp`, loader
`SLdrDestructibleBarrier.hpp/.inc`. Retail asm:
`build/G2ME01/DestructibleBarrier/asm/MetroidPrime/ScriptObjects/CScriptDestructibleBarrier.s`.
Per-function table: `DestructibleBarrier_functions.tsv` (B upstream body, H weak/implicit copy, T REL
runtime).

## Survey (db1, 2026-10-23)

`tools/upstream_hollow.py DestructibleBarrier`: nothing hollow. Every sub-100% body was diffed against
the retail asm (registers and stack offsets normalized).

| function(s) | verdict |
|---|---|
| SLdrDestructibleBarrier ctor fn_13_C1C | **fixed**: the five sound ids default to -1 (0xC38 `li r0,-0x1`, stores 0xC84..0xC94); upstream had 0. Retail's stores of 3 at +0x38, CColor(1,1,1,1) at +0x228 and 0xF at +0x278 repeat the SLdrEditorProperties/SLdrActorParameters sub-ctor defaults |
| SplitOffTop 98.7, DamageChunk 99.1, AddBoxGeometry 98.3, UpdateTransforms 98.8, PreRender 99.6, Load 99.8 | scheduling, stack, relocations; equivalent |
| grid Render 85.7 | hand-checked loop by loop against 0x43E4-0x476C; equivalent |
| asmcheck / retail_diff findings | bitfield NOPOS (mTouchedByPlayer x714_80, mHasTransparency x714_40 match declaration order), calls to inlined helpers and the unmarked ctor/TakeDamage/RenderChunks, branch counts from implicit member dtors and inlined loader readers |

## Shipped data

`scan_area_actors`: no DBAR placement in any area (the console lists DBAR as unplaced). The module is
data-dead in retail; all coverage is forced through `spawn DBAR`, as for ChozoGhost.

## Coverage (db1)

Coverage build, spec `tools/cov/specs/destructiblebarrier.py`, forced driver
`tools/drivers/destructiblebarrier_forced.con` (15 barriers A..O, editor ids e0036ff00..e0036ff0e, donor
CMDL/PART assets of the area), god mode:

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/destructiblebarrier.py <out> \
      42b935e4,08_sand_casualpirates --con tools/drivers/destructiblebarrier_forced.con --fast --frames 1200
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module DestructibleBarrier

0 caught/reach-stub. Every marked body is ALL-ARMS except the two dead-arm cases below; all arms are
`forced (spawn DBAR + damage/shoot/hurt/send/place)`.

| arm | how |
|---|---|
| projectile arm of TakeDamage | `shoot`, `shoot charged`, `shoot missile` |
| actor (non-projectile) arm | `damage` (player source, radius 20) |
| no source actor (670 cast fails) | `hurt` (no damager) on H |
| top split-offs, rebuilds | G, J..O, two hits per tick |
| lower / raise / lower again, touch while raising | A, B (`place` player inside), C/D delays, `send Reset` |
| center/right/full-row/full-layer/base model nulls | `prop <model> 0xffffffff` |
| grid Render center-chunk draw | I (width 4, damage in the middle column) |
| inactive early returns | `send Deactivate` / `Activate` on D |
| Delete while the move emitter is live | `send Reset; send Delete` on A, B |

Dead or unreachable arms:

- ApplyDamage 152 empty-callback arm: all three call sites (TakeDamage) pass a functor.
- GetCollisionPrimitive null fallback (770): mCollision only ever holds BuildCollision's rs_new result.
- BuildBoxNode: no `bl` to it in retail; unreferenced.
- Ctor left-model-null arm: on the host it dereferences null at `ModelData()->SetRenderUnsortedParts`
  first; rs_new throw arms are EH only.

Left unmarked:

- TakeDamage/Think/PreRender/RenderChunks: marked by db2 (below).
- Ctor: all lines ran, but gcov cannot attribute the six `IsNull() ? false : !IsDefinitelyOpaque()`
  ternary arms one by one.
- SetDamageVulnerability: only other modules call it; never ran.

## GRNT section arms (db2, 2026-10-23)

A GRNT connection (`conn 0x47524e54 <msg> <editor id>`) names a generated (SCGN) script object that
TakeDamage instantiates for each split-off top (`GenerateScriptObject`, asm 0x1FF4) while fewer than
four sections exist (0x1FB4 `lwz 0x664; cmpwi 4; beq`), moves it to `mRenderXf * Translate(spawnPos)`
(0x2028-0x2044), stores its id in the section (0x2048 `sth 0x214`), pushes the section and sends it
Activate ('ACTV', 0x205C/0x2074). Think drops sections whose actor is gone (0x2868-0x28AC); PreRender
(0x1330-0x13D8) and RenderChunks (0xFD4-0x11A4) bound and draw each section's grid at the actor's
transform less the pivot (main grid bounds 0x364, section grid 0x4(section), actor flags 0xFC). The
port matches all of it; no fix. Driver `tools/drivers/destructiblebarrier_grnt.con` (08_sand_casualpirates
SCGN: 00360244 a CScriptDebris, 00360245 a CScriptPickup), **1 tick per frame** (the gone-actor arms of
PreRender/RenderChunks need a render between the delete and the next Think; fast mode's 8 ticks per
render always let Think drop the section first):

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/destructiblebarrier.py <out> \
      42b935e4,08_sand_casualpirates --con tools/drivers/destructiblebarrier_grnt.con --frames 1000
    python3 tools/cov/gcov_report.py build-port-cov <db1 forced out>/gcov <out>/gcov --module DestructibleBarrier

| arm | how |
|---|---|
| non-GRNT connection skipped, GRNT found | A: `conn Down Activate` before the GRNT one |
| generated section, actor moved, Activate | A (debris), B (pickup) |
| four-section cap (0x1FB8 beq) | A's fifth split-off (hit chunk 0 after 8, 6, 4, 2) |
| no SCGN entry (0x200C beq) | C: GRNT to e0036ffff |
| sfx already played inside the split loop (0x2098 bne) | D: two `damage` + `send Damage` in one tick |
| Think erase / PreRender / RenderChunks gone actor | `send e00360245 Delete` (B's pickup) |

Notes: `damage` only queues the Damage message, so two `damage` in a tick add up to one TakeDamage;
`send <id> Damage` runs TakeDamage at once. TakeDamage's damage radius is 20 and aims at the damage
source, so the driver teleports the player to the height of the chunk to destroy.

Dead arms (db2):

- TakeDamage 0x201C `beq` (generated object not a CActor): an SCGN scan of all 291 areas' generated
  layers finds 19 types (DBR2 PCKP SOND MYSF MNNG EFCT ISSW SPBN SPLU SHRD SPLL SAFE PUFR GBUG MTDA
  WAYP VGOO RADD), all CActor-derived. Line 738's other zero pair is an EH landing pad.
- RenderChunks/GetChunkModel IsNull anim-data arm (e.g. 0x1040 `lwz r0,0x514(r25); cmplwi; bne`, the full-layer model's CAnimData): chunk models come
  from MakeChunkModel (CModelDataNull or CStaticRes), never an animated model.

Smoke family proposal `dbar`: area 42b935e4,08_sand_casualpirates, `tools/drivers/destructiblebarrier_forced.con`,
1200 frames, god 1.

## db3 (2026-10-24, base cab739771): loader dtor fn_13_BA0 marked

One launch (of the slice's two), `42b935e4,08_sand_casualpirates` (the module is data-dead, so the db1
forced driver's `spawn DBAR` path is the only way in), 600 frames, fast, `[port] caught/reach-stub: 0`,
inferior exited normally. Fixture `tools/cov/specs/db3_sldr_dtor.py` (read-only), mock
`db3_sldr_dtor_selftest.py`, own driver `db3_dbar_dtor.con` (three spawns).

Retail fn_13_BA0 (`__dt__23SLdrDestructibleBarrierFv`, weak, 0xBA0..0xC1C): 0xBB8 `mr. r30,r3` / 0xBBC
`beq` null-this guard, then the four member destructors (0xBC8 SLdrActorParameters +0x214, 0xBD4
SLdrDamageVulnerability +0xB8, 0xBE0 SLdrHealthInfo +0xB0, 0xBEC SLdrEditorProperties +0), then 0xBF0
`extsh. r0,r31` / 0xBF4 `ble` / `bl __dl__FPv` - the MWCC deleting-flag variant.

Host build: `_ZN23SLdrDestructibleBarrierD1Ev` == `D2Ev` at 0xde1b20 (four member calls, no null guard, no
flag), **no `D0Ev` symbol at all**, and the only call site in the binary is 0xde26fc in
`LoadDestructibleBarrier`'s `.cold` epilogue, with `this = lea -0x670(%rbp)` - the loader's own stack
object. Measured on the run: the normal path **inlines** the empty body, so the four member dtors run
with gdb showing an inlined `SLdrDestructibleBarrier::~SLdrDestructibleBarrier()` frame whose caller is
`LoadDestructibleBarrier`, root `0x7fffffffbb90` inside the loader frame, three times (one per spawn);
the out-of-line 0xde1b20 copy **never ran** (`outofline_hits=0`), i.e. the retail guard/delete variants
have no host counterpart on this path.

gcov (same run, `--module DestructibleBarrier`): `SLdrDestructibleBarrier::~SLdrDestructibleBarrier()`
header line 97, blocks 5/5 executed, `execution_count 3`, no branches.

Live paths: the member-dtor chain ran on every invocation; the retail null-this and delete-flag arms are
unreachable for this object (single caller-owned stack lifetime, non-null `lea`, object never deleted,
and the host build emits no deleting variant), so all live arms are justified. Marker
`// @ fn_13_BA0` added above the inline definition (header line 97).

Evidence `/tmp/opencode/db3_evidence`: `db3_r2.gcda` ef655298..., matching `db3.matching.gcno`
c6d42831..., pre-marker `SLdrDestructibleBarrier.source.hpp` 94a42706..., executed fixture
`db3_sldr_dtor.executed.py` 8da67562..., binary da8ea578..., `dtor_outofline.disasm.txt`,
`loader_callsite.disasm.txt`, `db3_r2_diag.log` (12 member-dtor lines), `db3_r2_fixture.txt`,
`db3_r2_raw.gcov.json`, `sha256.txt`.
