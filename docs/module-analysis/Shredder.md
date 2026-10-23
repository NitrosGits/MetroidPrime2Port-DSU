# Shredder REL (module 68): analysis notes

CShredder (SHRD, "Shredder") is a CPatterned that lurks inside a generator (Lurk). On Alert, when shot, or when it
hears a nearby noise (Listen), it generates out (Generate: up, or down for start state 2). It then flies at the
player (AttackPlayer) and explodes on contact (CollidedWith → Explode) or when its life time runs out. Dark
Shredders (`isDarkShredder`) use the dark variant. Code: upstream `src/MetroidPrime/Enemies/CShredder.cpp`, loader
`include/MetroidPrime/ScriptLoader/SLdrShredder.hpp`. Retail asm: `build/G2ME01/Shredder/asm/` (62 fns). Per-function
table: `Shredder_functions.tsv` (B upstream body, H weak/template copy or `__sinit`, T REL runtime). Note:
`config/G2ME01/rels/Shredder/symbols.txt` names fn_68_B20 `__ct__12SLdrShredder`; it is the SLdrShredderData ctor.

## Driver areas

| area (world 3dfd2249) | layer | objects | notes |
|---|---|---|---|
| 01_swamp_entrance | 3 | A=u04e6, B=u04e3, C=u04e2 | `tp 154 -80 23` is beside C; `shredder.con` |
| 0d_swamp_hall | 2 | u0438–u0443 (7 lurkers) | `tp 132 -30 37` beside u0442; `shredder_hall.con` |

All shipped Shredders in these rooms have start state 0 or 1, so start state 2 (downward generate) is forced.

## Survey (shr1, 2026-10-23)

| function | verdict |
|---|---|
| SLdrShredder ctor (inlined in LoadShredder, fn_68_168) | **fixed** (TARGET_PC, SLdrShredder.hpp:132-140): knockback resistance 2.0 (0x1D0 `stfs 0x488`, rodata 2.0); ambient (1,1,1,1) (0x1D4 CColor ct / 0x1F0 `stw 0x694`); visorFlags 0xf (0x1DC `li r0,0xf` / 0x1E0 `stw 0x6e4`). The `stw 1,0x668` creatureSize was already upstream |
| AcceptScriptMsg (fn_68_1EA0) | **fixed** (TARGET_PC, cpp:108-113): retail's XDMG case falls through into XRDG after Explode (0x1fe0 bl Explode, 0x1fe4 no branch) |
| Touch (fn_68_10B8) | **fixed** (TARGET_PC, cpp:299-305): heavy knockback for missile (0x10f8 cmpwi 6), charged (0x1104) or comboed (0x110c); upstream used only charged |
| every other body | equivalent to the asm |

Bit positions are now annotated for asmcheck: CShredder 0x808 (0x80 dark, 0x40 attacking, 0x20 woken, 0x10
lurking, 0x08 exploded) and `CPatterned::mHitByPlayerProjectile` (0x420 0x80). `tools/asmcheck.py check --summary
--module Shredder` exits 0. The only CALL findings left are inlined wrappers and the SLdr include in LoadShredder.

## Coverage (shr1)

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/shredder.py <a> 3dfd2249,01_swamp_entrance \
      --con tools/cov/specs/shredder.con --frames 1100
    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/shredder.py <b> 3dfd2249,0d_swamp_hall \
      --con tools/cov/specs/shredder_hall.con --frames 1100
    BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/shredder_force.py <f> 3dfd2249,01_swamp_entrance \
      --con tools/cov/specs/shredder_force.con --frames 800
    python3 tools/cov/gcov_report.py build-port-cov <a>/gcov <b>/gcov <f>/gcov --module Shredder

- `shredder.con`: god mode, Create and XDMG on C. Alerts A and B, shoots B with a missile and then a plain shot,
  and Damages A from the player's shot. Then it teleports beside C, Alerts it and shoots it. Result: generate, attack,
  Touch, Explode and AddReactionAnimation all run; the 01_swamp_entrance run makes 9 Explode, 13 Listen and 400 Touch calls.
- `shredder_hall.con`: u0442 is alerted with the player beside it. It hits the player and explodes. The noise makes u043e
  explode on the player too, and wakes u043b into Generate through Listen (the in-radius arm).
- `shredder_force.py/.con`: forces start state 2 on B (downward Generate, case 3) and 0 on A. It forces the detection
  range so ShouldWakeUp's InDetectionRange arm is taken, and toggles mDesiredDistance 1000/6.5 in AttackPlayer (both distance arms).

Every run: 0 `[port] caught` / `reach-stub`.

Open arms (cpp lines as of shr1), all data- or engine-dependent, none a divergence:

| line | arm | why open |
|---|---|---|
| 125 | random unavailable | load-time only |
| 138, 152, 193 | switch defaults | unreachable for shipped states |
| 169 | duration ≤ 0 | anim data always has a duration |
| 209 | cannot normalize | player never exactly at the Shredder |
| 232 | knockback while in fluid | no Shredder reached water in the drivers |
| 290 | comboed shot | the console cannot fire a combo |
| 337 | non-patterned in the ListeningAi list | only patterned AIs register |
| 368-377 | LoadShredder null / failed-token arms | needs broken data |

The ctor's line-72 else arm depends on the data.

## Markers

All 22 B bodies are marked: the 20 in CShredder.cpp (ctor, dtor, SetupStateMachine, AcceptScriptMsg, Lurk, Generate,
AttackPlayer, Think, CollidedWith, Explode, AddReactionAnimation, Touch, IsListening, Listen, ShouldWakeUp,
IsUnderWater, ApplySeparation, Render, LoadShredder, SetFuncPtrs), plus SLdrShredderData() (fn_68_B20) and
LoadTypedefShredderData (fn_68_838) in SLdrShredder.hpp. In fn_68_838 every property hash maps to the same field
offset as the asm. Smoke family: `shrd` = 3dfd2249,01_swamp_entrance, `tools/cov/specs/shredder.con`, 1100 frames.
