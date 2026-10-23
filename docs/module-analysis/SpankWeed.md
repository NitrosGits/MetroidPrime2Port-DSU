# SpankWeed REL (module 73): analysis notes

CSpankWeed (SPNK): a patterned plant enemy that hides and lunges. Think pins its HP at 1000000, moves the
collision primitive to the "Eye" locator while it is not hiding, and updates a CCollisionActorManager
built from its joints. KnockBack only applies while mCanKnockBack is set. Source: upstream
`src/MetroidPrime/Enemies/CSpankWeed.cpp`, loader `SLdrSpankWeed.hpp/.inc`. Retail asm:
`build/G2ME01/SpankWeed/asm/MetroidPrime/Enemies/CSpankWeed.s`. Per-function table:
`SpankWeed_functions.tsv` (B upstream body, H weak/template copy, T dead or REL runtime).

## Survey (spw1, 2026-10-23)

Every B body was compared with the retail asm.

| function(s) | verdict |
|---|---|
| Think fn_73_1D48 | **fixed** (TARGET_PC): `0x1DD0 bne .L_00001E70` skips only the eye block. The `Update` at 0x1E80 runs on every active frame, with `0x1E7C li r5,0x1` (kUO_WorldSpace). Upstream called it inside the eye block with kUO_ObjectSpace |
| ctor, AcceptScriptMsg, KnockBack, GetAimPosition, GetOrbitPosition, Render, LoadSpankWeed, dtor, SLdr ctor/dtor | equivalent. The loader property ids match the retail compares (0x255a4580, b3774750, 9b5a4744, 835adca1, ed9bf5a3, 7c11b995, 5cdc877d, 7e397fed) |
| asmcheck findings on marked bodies | calls to the inline string dtor, ~CPatterned, Free, the SLdr ctor, the optional_object copy, the ctor and the loader typedef readers. The 19-vs-4 branch count comes from the `.inc` property switch |

## Retail-dead code

- **No states or triggers are registered.** The vtable keeps `SetupStateMachine__10CPatterned`, and upstream has no override, so the port registers none either. Patrol, FadeIn, FadeOut, Lurk, TargetPatrol, Attack, Flinch, InDetectionRange, InRange, HearPlayer, Delay, AnimOver, Attacked and IsPlayerNear (called only by the triggers) are unreachable (T).
- **CSpankWeedCollisionActor is never constructed.** There is no `bl __ct__24CSpankWeedCollisionActor`; the only internal call is its own Think → UpdateBounds (0x3554). All its functions are T.
- **mState (0x80c) and mPreviousState (0x810)** are written only by the ctor (-1, 0x2BCC/0x2BD0) and by the unregistered states. So GetOrbitPosition's 3/2 and 2/3 lerp arms are dead. The `min(time, 1)` at 0xE10 feeds only those arms (f7 is unused on the fallthrough at 0xED8).
- **mCanKnockBack (0x7d4)** is set to 1 only in FadeIn (0x1AA4 `li r3,1` / 0x1AB0 `stb`). It is cleared at ctor 0x2B78, FadeOut 0x18D0 and KnockBack 0x1D30. So KnockBack's apply arm is dead.
- **mIsHiding (0x7f0)** is cleared only in Patrol's Deactivate (0x1CDC, unregistered). So Think's eye block is retail-dead; the coverage run forces it.

## Shipped data

`python3 tools/scan_spnk_props.py`: `== 0 SPNK placements in 291 areas, 71819 objects, 168 types`. The
module is data-dead in retail, so all coverage is forced through `spawn SPNK`.

## Coverage (spw1)

The coverage build uses spec `tools/cov/specs/spankweed.py` and the forced driver `tools/drivers/spankweed_forced.con`:

- u0535 / e0036ff00 has a borrowed SpacePirate ANCS u04d0, with wake/search/attack radii 30/20/10 and hurtSleepDelay 2.
- A second `spawn SPNK` with no ANCS takes the loader's null-model arm.
- The run uses god mode.

Commands:

    COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/spankweed.py <out> \
      42b935e4,08_sand_casualpirates --con tools/drivers/spankweed_forced.con --fast --frames 900
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module SpankWeed

Result: rc 0, 0 caught/reach-stub.

| marked body | arms |
|---|---|
| Think fn_73_1D48 | ALL-ARMS. Eye block `forced (gdb: mIsHiding = 0 at Think entry)`; inactive return `forced (send Deactivate)` |
| LoadSpankWeed fn_73_494 | ALL-ARMS. Null model `forced (spawn SPNK, no ancs)` |
| KnockBack fn_73_1D08 | the false arm ran (`damage ... kb missile`); the true arm is dead (see above) |
| GetOrbitPosition fn_73_DAC | the fallthrough ran; the lerp arms and the min arm are dead (see above) |
| dtor fn_73_0 | ran (`send Delete`) |

Unmarked bodies, with the arms that never ran:

- ctor: non-uniform scale (177), lockon locator found (195).
- AcceptScriptMsg: the Create spatial-primitive loop, the HitObject collision-actor arm and other message arms.
- GetAimPosition: lockon locator found (533).
- Render: the spatial-primitive arms (568-574).

The borrowed body has no `lockon_target_LCTR` locator and no spatial primitive. No placement names a SpankWeed
ANCS, so these arms need a donor model that has both.
