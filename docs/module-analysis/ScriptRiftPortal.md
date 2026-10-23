# ScriptRiftPortal REL (module 64): analysis notes

CScriptRiftPortal (RPTL, "Rift Portal") is the portal at the end of the game. Activate draws it and Increment plays the
line animation until it opens. While it is open, Render copies the screen region the portal covers into the spare
texture and draws it back distorted. UpdateProjectiles pulls projectiles inside its projectile box toward its centre
and destroys them there. The module's code is upstream's `src/MetroidPrime/ScriptObjects/CScriptRiftPortal.cpp`, with
the loader `include/MetroidPrime/ScriptLoader/SLdrRiftPortal.hpp`. The retail asm is
`build/G2ME01/ScriptRiftPortal/asm/auto_00_00000000_text.s` (34 fns). `ScriptRiftPortal_functions.tsv` has one row per
function (B upstream body, H weak/template copy, T REL runtime).

There are two placements (`tools/scan_riftportal_props.py`). Temple Grounds `12_temple_finalgate` L0 has e00070073 at
(4.5, 132.7, -16.6). `game_end_part1` L2 has e082f009e. Both are inactive at load, and every property the arms read
is identical in the two: ripPortal 1, projectileAttraction 1, box 30, angle 600, radius 2. Main, incandescent and line
models have an ANCS, and the background model has none.

## Survey (srp1, 2026-10-23), commit 5a9f2d369

| function | verdict |
|---|---|
| ctor (fn_64_28B4) | **fixed** (TARGET_PC, cpp:63). Retail sets mDrawEnabled and mRenderParticleDBInside (0x2A70-0x2A84 `rlwimi`/`stb` 0x153/0x152). It then re-places the transform: Rotate(0, scale.x, 0) and RotateLocalX(pi/2) (0x2A90-0x2AEC), with `bl SetTransformAlt` at 0x2B04 |
| Render (fn_64_C88) | **fixed** (TARGET_PC, cpp:306-491), with these GX constants:<br>- The copy and the spare texture are RGBA8 (0x1188, 0x1A3C `li r5,0x6`).<br>- The tex matrix uses rows 0, 1 and 3 of the 4x4 (0x1428-0x143C).<br>- The tex coord source is GX_TG_TEX0 (0x1488/0x1924 `li r5,0x4`).<br>- The KColor selector is 1_8 (0x14B8/0x1954 `li r4,0x7`).<br>- The blend is (Blend, DstAlpha, InvDstAlpha, Clear) (0x17E8-0x17F4) |
| LoadRiftPortal (fn_64_128) | **host guard** (TARGET_PC, cpp:550). Retail checks only the main model (0x550 `lbz 0x2cc`, `bne` → new, else return 0 at 0x5F8). The host also requires the background model, because Render dereferences it unguarded. A missing incandescent or line model becomes a null CModelData instead of an unconstructed one |
| every other body | equivalent to the asm |

## Coverage (srp1)

    BUILD=$PWD/build-port-cov SECS=900 tools/cov/run.sh tools/cov/specs/riftportal.py /tmp/opencode/srp1_c \
      3bfa3eff,12_temple_finalgate --con tools/cov/specs/riftportal.con --frames 2040

The `riftportal.con` driver runs these steps, in order:

1. It activates the portal and plays its opening animation (Increment).
2. It shoots the portal: beam, missile, charged shot and a 2-shot burst through the projectile box.
3. It sends AreaLoaded, then Decrement and Deactivate, then Activate and Increment again.
4. It steps close and turns the portal centre off-screen left (face 40/320/65/295), which takes the clampedX min arm.
5. It looks away to cull the portal.
6. It drops from above.
7. It lays morph-ball bombs in the box. A bomb is a kMT_Projectile actor that is not a CGameProjectile, which is the `continue` arm.

The run reports 0 `[port] caught` and 0 `reach-stub`, and exits with 0.

Marked: the ctor, the dtor, LoadRiftPortal, SetActive, PreRenderAllViewports, PreRender, CopyFrameBufferRegion,
AddToRenderer, AcceptScriptMsg, Think and UpdateProjectiles, plus SetFuncPtrs, RELMain and RELExit, which are trivial
and equivalent to the asm. These three are not built in the port (MONOLITHIC); they are marked as in Kralee. Every
0% arm in a marked body is dead for one of these reasons:

| reason | arms (cpp line) |
|---|---|
| data: both placements have the same properties, and the only writer is the loader | UpdateProjectiles attraction 0 (86) and 2 (111). Every `HasAnimation()` false arm: Think 146/154/158/165/168, AcceptScriptMsg 178-238, Render 441/483/487/502, PreRender 513. Render `!mRipPortal` (274) and AddToRenderer `mRipPortal` false (251). Load's missing-model arms and the LdrToModelData empty-optional arms |
| caller: CStateManager.cpp:824 calls Render/AddToRenderer only for active actors with models | Render `IsNull`/`!GetActive` (274), AddToRenderer `GetActive` false (251) |
| caller: the only call passes halfSize false and dest null | CopyFrameBufferRegion 265/267 |
| arithmetic | UpdateProjectiles 109 (CanBeNormalized false inside a 2-unit radius with toCenter != 0); Render 345 (Clamp of offsets already in [-1,1]) |
| writer: the CModel ctor always sets mModelInstance (DolphinCModel.cpp:307/328) | Render `cubeModel == nullptr` (414/439; retail `cmplwi r29,0` at 0x168C) |
| compiler artifacts | inline deref/get helper arms, the static-guard arm, EH "throw" arms |

Open (unmarked):

| function | open arm |
|---|---|
| Render C88 | 299, the clampedY min arm (`maxY < -1`): every corner must be below the screen while the portal's point bounds stay visible. Close-ups, drops and an `aim 0 0..60` pitch sweep at 12, 6 and 4 units did not reach it |
| AdvanceModel 23F8 | 130, the POI loop: the particle POI count was always 0. It depends on the ANCS EVNT data, which has not been scanned |
| CanRenderUnsorted C30 | never called |

## Smoke family (proposal)

`rptl`: area `3bfa3eff,12_temple_finalgate`, driver `tools/cov/specs/riftportal.con`, 2040 frames, not fast. Expect
`obj e00070073` to show `inactive` at f300 and active after f340. The pass condition is 0 `[port] caught` and 0
`reach-stub`.
