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

- Think 545-549, TakeDamage 701-723, PreRender 802-806 and RenderChunks 876-882 need a section, i.e. an
  SConnection with state GRNT to a generator. `spawn` cannot add connections (needs a console option).
- TakeDamage 726 (sfx already played inside the split loop): needs two split-offs from one hit.
- Ctor: all lines ran, but gcov cannot attribute the six `IsNull() ? false : !IsDefinitelyOpaque()`
  ternary arms one by one.
- SetDamageVulnerability: only other modules call it; never ran.

Smoke family proposal `dbar`: area 42b935e4,08_sand_casualpirates, `tools/drivers/destructiblebarrier_forced.con`,
1200 frames, god 1.
