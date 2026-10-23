# ChozoGhost REL (module 8): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CChozoGhost.cpp`; the loader defaults are
`include/MetroidPrime/ScriptLoader/SLdrChozoGhost.hpp` (the port's fix is a `TARGET_PC` block there).
Retail asm: `build/G2ME01/ChozoGhost/asm/MetroidPrime/Enemies/CChozoGhost.s`. Per-function table:
`ChozoGhost_functions.tsv` (B = module body, H = weak copy/template or inline ctor, T = REL runtime).

## Shipped data: the module is dead

No shipped area places a CHOG object, no area's module list names ChozoGhost.rel, and no AFSM holds
the ChozoGhost states (Growth, Hurled, WallDetach) or the AIStage trigger. Echoes never loads this
REL (`docs/modules.tsv`: 0 areas). All the coverage below is forced with console `spawn CHOG`.

## Behaviour fixes (cg1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_8_FC0 SLdrChozoGhost ctor | 0x1098 `li r12,-1`; 0x110C/0x1110/0x1114/0x111C `stw r12` 0x384/0x38c/0x390/0x3a8: the Impact, PhazeIn, PhazeOut and ProjectileVisor sounds -1 | 0 (a real sfx id) |
| same | 0x1150 CColor(1,1,1,1), 0x1164 `stw 0x294` ambient; 0x1168/0x1174 `stw 0xf,0x2e4` visorFlags | 0 / 0 |
| same, inlined SLdrGhostBehave ctor | 25,25,25,25,2,50,1 per range (0x1024-0x109C), then 0.0 for far heckle/chargeAttack and near lurk/heckle/move (0x11FC-0x1258) | the final values (unchanged) |

Checked equivalent (no fix): objdiff is 100% for every CChozoGhost body except AIStage (99%: the
`subf` operands swapped, same value), LoadChozoGhost (99.4%, regalloc and rodata labels) and
PreRender (91%: regalloc; both builds store an uninitialised CModelFlags word, no reader).
`upstream_hollow.py ChozoGhost`: none.

## Coverage (cg1)

`spawn CHOG` needs three stand-ins: the borrowed SpacePirate body `ancs u04d0 1 34` (character 1:
the donor's `GetCharacterIndex()` comes back 0, whose PAS database is empty, and the body controller
then starts state -1 and crashes), the pirate AFSM `fsm 5652a12f`, and the pirate WPSC 681f3907 for
both projectile props (`prop 54151870`, `prop 3a58089c`: the -1 defaults crash
CProjectileWeapon on the first kUE_Projectile). The PatternedAITypedef's knockbackRules stays -1;
the missing RULE reads as zeros and its parent rule 0 recurses until the heap runs out, so the run
writes RULE 393ca543 into an unset knockbackRules at `LdrToPatternedInfo` (gdb,
`tools/cov/specs/chozoghost_force.py`). Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`, then:

    COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov tools/cov/force.sh \
      tools/cov/specs/chozoghost_force.py <out> 42b935e4,08_sand_casualpirates \
      --con tools/drivers/chozoghost_forced.con --frames 1000
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module ChozoGhost

Not `--fast`: the driver needs `frame` breaks (the console queues at most 256 commands), and an
unpaced game outruns each break by hundreds of updates. The driver runs every state body
(Activate, an Update per frame for 14 frames, Deactivate), every trigger, the script messages, user
anim events 0/3/6/14/15, the echo/scan/dark visors, contact with the player, beam/missile/charged
hits and kills, and a `spawn CHOG` without a body (the loader's null arm).

Marked by cg1 (29; every arm taken or branch-free with every line run; all forced (`spawn CHOG`),
objdiff as above): CBehaveChance ctor, AcceptScriptMsg, CanBeShot, GetCollisionResponseType,
GetDamageVulnerability, GetModelAlphau8, Render, IsVisibleEnough, FloatToLevel, IsOnGround,
ChooseBehaveChanceRange, SetWarpPosition, AIStage, Growth, InRange, SelectTarget, ShouldAttack,
ShouldTaunt, Lurk, ShouldMove, ShouldFlinch, AggressionCheck, Dead, PreThink, Think,
SetupStateMachine, LoadChozoGhost, RELMain, RELExit.

| function | arms | how |
|---|---|---|
| AcceptScriptMsg | OffGround with `!mVerticalMovement` | forced (`statecall Hurled 0`, then `send OffGround`) |
| ChooseBehaveChanceRange | mid range | forced (`spawn ... prop 61e511b3 0.5 prop 2369607a 1.0`, `place ... player 0.75`) |
| LoadChozoGhost | `!modelData` | forced (`spawn CHOG` without `ancs`) |
| GetDamageVulnerability | `x665_24_` false | forced (`statecall Growth 2` before the hits) |
| Dead | 0/1/2 | forced (`statecall <id> Dead 0/1/2`) |

Unmarked (live arms not run): the CChozoGhost ctor (the projectileVisor PART arm: needs a PART id
in the area), GetBehave (one `switch (type)` arm), Touch (one arm), PreRender (the red-tint arms),
InActive (body controller already on: `spawn` activates it), Generate, WallDetach, Run, Attack,
Land, Taunt, Hurled, Deactivate (CanIssueCommand / animation-progress arms the borrowed pirate body
does not reach), Leash (`mPlayerInLeashRange`, set by FindBestAnchor), ProjectileInfo and
DoUserAnimEvent (the attack-type-2 arm, the `speed > 0` arm), KnockBack (the charged Hurled range:
the borrowed rules keep the KnockBack reaction on), FindBestAnchor, AddToTeam, RemoveFromTeam,
GetOrigin and Shuffle's role arm (they need script anchors and a TMAI team connection, which a
spawned ghost has none of).

Host note: Shuffle's `Next() % numBolts` would raise SIGFPE on the host when numBolts is 0 (PPC
`divwu` does not trap). No guard was added, because no shipped data reaches it.
