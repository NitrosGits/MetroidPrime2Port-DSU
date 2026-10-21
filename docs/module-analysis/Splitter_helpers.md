# Splitter REL (module 75): the stateless helpers (this lane's half)

This note is the helpers lane's half of the Splitter module: the module's **behaviour functions
that are not FSM handlers** - the bodies the two classes' vtable slots and the module's own
objects call. The module's own note is `docs/module-analysis/Splitter.md`; this lane does not
edit it, and it does not edit the FSM tables (the states, the triggers and the codes belong to
the Opus lane on `decomp/splitter`).

The ported bodies live in `src/MetroidPrime/Enemies/PortCSplitterHelpers.cpp`; the declarations
are in one append-only `// helpers` block at the end of the class in
`include/MetroidPrime/Enemies/CSplitterCommandModule.hpp` (the head's five) and in the same
kind of block at the end of the class in `include/MetroidPrime/Enemies/CSplitterMainChassis.hpp`
(the chassis's three). Every bit test, mask, compare and constant carries the retail asm offset
it came from.

## 1. How each row was classified

Three scans over the module's own build in the archive tree
(`<archive>/build/G2ME01/Splitter/asm`):

1. **Direct calls.** `bl fn_75_XXXX` over the module's `.text`: the retail call sites.
2. **Table references.** The module's `.data` (the three vtables and the six FSM registration
   tables). The FSM rows are the ones the module's static initialisers (`fn_75_7C80` for the
   head, `fn_75_FED0` for the chassis) put into the tables.
3. **The port's state.** What the tree already has for each row: a body (a `Port*.cpp` def whose
   comment cites the offset), a stand-in, a declaration only, or nothing.

The **kind** column uses the Sandworm note's rule set (`docs/module-analysis/Sandworm.md`
section g), with one extra class for this module:

| class | meaning | the port |
|---|---|---|
| **F** | an FSM handler (a state, a trigger or a code of one of the six tables) | the Opus lane's |
| **H** | a host copy: the body is a copy of something the host already has (the crtexe REL runtime, the loaders and their `SLdr`/`LdrTo` helpers, the REL's own copies of the host's `CModelData`/`CDamageInfo` members, the compiler-emitted destructors) | no body is written |
| **T** | a data / vtable thunk (the body only returns a constant, a member address or a flag) or a function the REL never calls (no module text calls it and no table names it) | no body is written |
| **B** | module behaviour: the real bodies | this lane's and the Opus lane's |

The counts (all 307 symbols of `config/G2ME01/rels/Splitter/symbols.txt`):

| class | functions |
|---|---|
| F FSM handler | 99 |
| H host copy | 60 |
| T data / vtable thunk or dead | 61 |
| B module behaviour | 87 |
| **total** | **307** |

Of the 87 B rows, 31 carry a marker today and 56 do not; section 2 lists the 56. This lane
ports the head's five (section 3) and marks the four whose every live arm ran, plus the
chassis's `fn_75_8EA4` (section 3.3), whose copy ran in the shipped spawns.

## 2. The unmarked B rows

The queue: the module's behaviour functions that carry no marker. The three the FSM lane
ported in 1278a3447 (`fn_75_E99C`, `fn_75_E90C`, `fn_75_E810` - the chassis's
CanBeIngPossessed, SetIngPossessed and SetAttackTarget) are **the FSM lane's**: they live in
the class's own bodies in `PortCSplitterMainChassis.cpp` and the lane marked them after the
Hive Temple and the contraption runs. This lane's copies of them are gone; the helpers file
keeps the head's five.

| id | size | the retail's callers | a table names it |
|---|---|---|---|
| `fn_75_7EC` | 0x3C | fn_75_E60 |  |
| `fn_75_828` | 0x148 | fn_75_F00 |  |
| `fn_75_E60` | 0xA0 | fn_75_6798 fn_75_FC |  |
| `fn_75_F00` | 0x1D0 | fn_75_FC |  |
| `fn_75_14CC` | 0x58 | fn_75_1188 |  |
| `fn_75_1524` | 0x54 | fn_75_14CC |  |
| `fn_75_1578` | 0x218 | fn_75_1790 fn_75_1974 fn_75_31A8 fn_75_3E60 fn_75_523C fn_75_5B28 fn_75_5CC4 fn_75_5D50 fn_75_5DDC |  |
| `fn_75_1C0C` | 0x94 | fn_75_1974 |  |
| `fn_75_1CA0` | 0x198 | fn_75_6394 |  |
| `fn_75_1E38` | 0x54 | fn_75_1CA0 |  |
| `fn_75_1E8C` | 0x5C | fn_75_1E38 fn_75_1F50 |  |
| `fn_75_1EE8` | 0x20 | fn_75_1E8C |  |
| `fn_75_1F08` | 0x48 | fn_75_1EE8 |  |
| `fn_75_1F50` | 0x48 | fn_75_1CA0 |  |
| `fn_75_1F98` | 0xB4 | fn_75_1F50 |  |
| `fn_75_204C` | 0x40 | fn_75_1F98 |  |
| `fn_75_208C` | 0x20 | fn_75_204C |  |
| `fn_75_20AC` | 0x28 | fn_75_208C |  |
| `fn_75_20D4` | 0x80 | fn_75_20AC |  |
| `fn_75_2154` | 0xA8 | fn_75_3FE0 fn_75_4138 fn_75_4D04 fn_75_4E90 |  |
| `fn_75_2280` | 0x170 | fn_75_31A8 |  |
| `fn_75_2994` | 0x24C | fn_75_42F8 |  |
| `fn_75_2BE0` | 0xE0 | fn_75_3004 |  |
| `fn_75_2CE0` | 0x324 | fn_75_3004 fn_75_3460 |  |
| `fn_75_3004` | 0x1A4 | fn_75_6490 |  |
| `fn_75_3678` | 0x28 | fn_75_3460 fn_75_A4E4 |  |
| `fn_75_36A0` | 0x7C | fn_75_3678 |  |
| `fn_75_371C` | 0x114 | fn_75_36A0 |  |
| `fn_75_3830` | 0x38 | fn_75_3460 fn_75_A4E4 |  |
| `fn_75_3868` | 0x20 | fn_75_3830 fn_75_7BDC |  |
| `fn_75_3888` | 0x28 | fn_75_3868 fn_75_AA10 |  |
| `fn_75_38B0` | 0xC0 | fn_75_3460 fn_75_3888 fn_75_A4E4 |  |
| `fn_75_3970` | 0x84 | fn_75_3460 fn_75_A4E4 |  |
| `fn_75_39F4` | 0x38 | fn_75_3970 |  |
| `fn_75_3A2C` | 0x60 | fn_75_39F4 fn_75_AA30 |  |
| `fn_75_5B9C` | 0x5C | fn_75_5B28 fn_75_5CC4 fn_75_5D50 fn_75_5DDC |  |
| `fn_75_6890` | 0x1E4 | fn_75_6798 fn_75_F560 |  |
| `fn_75_6A74` | 0x94 | fn_75_6890 |  |
| `fn_75_8EA4` | 0x24 | fn_75_BE28 fn_75_E6E4 |  |
| `fn_75_91E4` | 0x33C | fn_75_B2A0 |  |
| `fn_75_9520` | 0x11C | fn_75_F0E8 |  |
| `fn_75_986C` | 0x28C | fn_75_DC08 fn_75_DCFC |  |
| `fn_75_9AF8` | 0xA4 | fn_75_B9B0 fn_75_BAA8 fn_75_C9E4 fn_75_D3E0 fn_75_EFA4 |  |
| `fn_75_A0A0` | 0x2DC | fn_75_F1C0 |  |
| `fn_75_A490` | 0x54 | fn_75_A37C |  |
| `fn_75_AA10` | 0x20 | fn_75_FE68 |  |
| `fn_75_ACEC` | 0x2FC | fn_75_F0E8 |  |
| `fn_75_AFE8` | 0x11C | fn_75_F0E8 |  |
| `fn_75_C3A8` | 0x9C | fn_75_C034 |  |
| `fn_75_C4A0` | 0x7C | fn_75_C034 |  |
| `fn_75_CEE8` | 0x5C | fn_75_CCF0 |  |
| `fn_75_E68C` | 0x58 | fn_75_3BEC fn_75_4138 |  |
| `fn_75_E6E4` | 0x84 | fn_75_3EEC |  |
| `fn_75_E768` | 0x20 | fn_75_2280 |  |
| `fn_75_E788` | 0x88 | fn_75_2280 |  |
| `fn_75_F5F8` | 0x538 | fn_75_82B0 |  |
| `fn_75_11A8C` | 0x20 | fn_75_11884 |  |

## 3. The helpers this lane ports

All 5 bodies are in `src/MetroidPrime/Enemies/PortCSplitterHelpers.cpp`; the declarations are
in the append-only helpers block at the end of `CSplitterCommandModule.hpp`.

| id | size | what | the class | marked |
|---|---|---|---|---|
| `fn_75_6310` | 0x2C | the head's AddToRenderer: the base class's call only while the actor is alive | head | yes |
| `fn_75_633C` | 0x2C | the head's PreRender: the same alive gate | head | yes |
| `fn_75_6368` | 0x2C | the head's Render: the same alive gate | head | yes |
| `fn_75_5E50` | 0x3C | the head's CanBeIngPossessed: only while it is docked (the chassis's id at 0xEFA) | head | no |
| `fn_75_5F18` | 0x74 | the head's Listen: alive, the noise type 4 and the distance (20 units) | head | yes |

Five stand-in call sites now call them instead of logging: the head's `PreRender`,
`AddToRenderer`, `Render`, `Listen` and `CanBeIngPossessed` (`PortCSplitterCommandModule.cpp`).

The port's stand-ins called the base class **unconditionally**, so the head drew and rendered
itself while it was dead and the head was possessable while it was docked: four of the five
bodies are behaviour fixes, not just new code.

### 3.1 The run

**04_cliff_contraption** (world 1baa96c2, area 914f1381), **layer 2** - the fight with the
chassis u0532, its head u0531, the chassis u0521 and the head u0520. The coverage harness:

    tools/cov/run.sh tools/cov/specs/splitter_helpers_head.py /tmp/opencode/sph_cov_h4 \
      1baa96c2,914f1381 --driver "MP2_CONSOLE={port} LAYER=2 PLAN=Z python3 \
      tools/drivers/splitter_helpers_drv.py /tmp/opencode/sph_cov_h4/game.log 200 40" \
      --fast --frames 20000

The game exited with code 0 and the log has **0 `[port] caught` and 0 `reach-stub`** lines.

The one arm the fight cannot reach - the head's Listen with a dead head - was forced with the
same harness:

    tools/cov/force.sh tools/cov/specs/splitter_helpers_force.py /tmp/opencode/sph_force \
      1baa96c2,914f1381 --con tools/cov/specs/splitter_helpers.con --frames 20000

The run's `forced.log` names the forced hit ("the head is not alive: the Listen returns false
without asking the distance") and `forced.txt` lists it.

### 3.2 The arms

| id | the arms | how they ran |
|---|---|---|
| `fn_75_6310` (the head's AddToRenderer) | (a) the actor is alive: the base class's call runs, (b) it is not: nothing runs | **both** ran: 800 entries, the base class's `CPatterned::AddToRenderer` call 400 times (the `calls` key) and 400 entries without it. **Marked** |
| `fn_75_633C` (the head's PreRender) | the same two | **both** ran: 800 entries, `CPatterned::PreRender` 400 times. **Marked** |
| `fn_75_6368` (the head's Render) | the same two | **both** ran: 800 entries, `CPatterned::Render` 400 times. **Marked** |
| `fn_75_5F18` (the head's Listen) | (a) the actor is not alive: false, (b) the noise type is not 4: false, (c) the noise type is 4 and the distance is 20 or more: false, (d) the noise type is 4 and the noise is inside the radius: the flag byte at 0xF6A gets its 0x40 bit and the body returns true | (b) ran (the `noise 3` call, 2 entries), (c) ran (the `noise 4` call from 60 units up, 7 entries), (d) ran (the `noise 4` call next to the head, 2 entries, the return value 1). (a) cannot run in play - a dead head is deleted, so the noise system never asks it - and was **forced**: the alive flag is written at the body's entry for one call (`tools/cov/force.sh` with `tools/cov/specs/splitter_helpers_force.py`, the `forced.log` in the run's directory). **Marked** |
| `fn_75_5E50` (the head's CanBeIngPossessed) | (a) the head is docked: false, (b) it is not: the base class's answer | the body was never called in the run (the Ing possession system does not ask in this area): 0 entries. **Unmarked** |

The four marked bodies' arms are also in the **coverage** profile, which is the stronger
evidence: one forcing run per body (`SPLITTER_FORCE=<body>`, `COV_GDB_SCRIPT=tools/cov/cov_force.py`,
`BUILD=build-port-cov`) writes the alive flag at the body's entry, and the runs' counters add up:

    SPLITTER_FORCE=6310 COV_GDB_SCRIPT=$PWD/tools/cov/cov_force.py BUILD=$PWD/build-port-cov \
      tools/cov/force.sh tools/cov/specs/splitter_helpers_force.py /tmp/opencode/gcov_f_6310 \
      1baa96c2,914f1381 --con tools/cov/specs/splitter_hive_temple.con --fast --frames 12000
    (the same for 633C, 6368 and 5F18 - one body per run, because the force makes the head dead
    and a dead head is not rendered any more, so one gate's force would keep the others uncalled)

    tools/cov/gcov_report.py build-port-cov /tmp/opencode/gcov_temple/gcov \
      /tmp/opencode/gcov_hive/gcov /tmp/opencode/gcov_contr/gcov /tmp/opencode/gcov_f_6310/gcov \
      /tmp/opencode/gcov_f_633C/gcov /tmp/opencode/gcov_f_6368/gcov /tmp/opencode/gcov_f_5F18b/gcov \
      --module Splitter

| id | the profile | arms |
|---|---|---|
| `fn_75_6310` (the head's AddToRenderer) | 4/4 lines, 2/2 branches | **ALL-ARMS** |
| `fn_75_633C` (the head's PreRender) | 4/4 lines, 2/2 branches | **ALL-ARMS** |
| `fn_75_6368` (the head's Render) | 4/4 lines, 2/2 branches | **ALL-ARMS** |
| `fn_75_5F18` (the head's Listen) | 7/7 lines, 8/8 branches | **ALL-ARMS** |
| `fn_75_5E50` (the head's CanBeIngPossessed) | 0/3 lines | never called |

The gdb counters are the evidence for the three alive gates: the `calls` key counts the
hits at the base class's call site (400) and the entry key the hits of the body itself (800),
so 400 entries took the gate and 400 did not. The coverage build's profile of the three
shipped areas (section 3.3) shows only the alive arm: the not-alive arm needs a head that is
not alive, and the shipped areas' heads are alive the whole time.

### 3.3 The coverage build: what ran in the shipped spawns

The gcov build (`-DMP_PORT_COVERAGE=ON`, its own `build-port-cov`) and the report tool
(`tools/cov/gcov_report.py <build> <prefix> --module Splitter`) say which of the module's
bodies the **shipped** spawns reach. The runs: **09_cliff_temple** (6c363cd4, layer 2 - the
swallowing swarm's spawn, `GENR 82500e1` on `TIMR 82500e4`), **09_cliff_temple_dark** (a92f00b3,
the Hive Temple, layer 10 - the generated `SPLL`/`SPLU` through the relays `SRLY
28350006/11/28`) and **04_cliff_contraption** (914f1381, layer 2). The three profiles add up:

    tools/cov/gcov_report.py build-port-cov /tmp/opencode/gcov_temple/gcov \
      /tmp/opencode/gcov_hive/gcov /tmp/opencode/gcov_contr/gcov --module Splitter

81 bodies ran. The queue rows among them and their arms:

| id | the port's body | the profile | marked |
|---|---|---|---|
| `fn_75_8EA4` (the chassis copies its four timers) | `CSplitterMainChassis::PortCopyTimers` | 6/6 lines, no branch untaken | **yes** (this lane added the marker) |
| `fn_75_10D0` (the module's own transform) | `CSplitterCommandModule::PortModuleTransform` | 6/6 lines, 2/2 branches, ALL-ARMS | the FSM lane's marker |
| `fn_75_3A8C` (the connected-object lookup) | `CSplitterCommandModule::PortUpdateConnectedObject` | 3/3 lines | the FSM lane's marker |
| `fn_75_2CE0` (the head's shield) | `CSplitterCommandModule::PortUpdateShield` | 5/34 lines, 3/18 branches | no |
| `fn_75_2BE0` (the shield's effect) | `CSplitterCommandModule::PortGetShieldEffect` | 2/4 lines, 1/5 branches | no |
| `fn_75_31A8` (the chassis check) | `CSplitterCommandModule::PortUpdateChassis` | 8/22 lines, 4/22 branches | no |
| `fn_75_32F4` (the flash and the death) | `CSplitterCommandModule::PortUpdateFlashAndDeath` | 9/17 lines, 2/10 branches | no |
| `fn_75_21FC` (the auto-destruct timer) | `CSplitterCommandModule::PortAutoDestructTimer` | 4/8 lines, 1/4 branches | no |
| `fn_75_184C` (the look-at) | `CSplitterCommandModule::PortLookAtTarget` | 4/12 lines, 1/2 branches | no |
| `fn_75_58E8` (the chassis's scan timer) | `CSplitterCommandModule::PortChassisTimerRunning` | 5/5 lines, 1/2 branches | no |
| `fn_75_1188` (the beam-effect spawner) | `CSplitterCommandModule::PortSpawnBeamEffect` | 11/36 lines, 3/21 branches | no |
| `fn_75_1790` (the laser's despawn) | `CSplitterCommandModule::PortDespawnProjectile` | 7/12 lines, 3/8 branches | no |
| `fn_75_3460` (the head's collision actor manager) | `CSplitterCommandModule::PortCreateCollisionActorManager` | 18/20 lines, 21/24 branches | no |
| `fn_75_654C` (the head's messages) | `CSplitterCommandModule::AcceptScriptMsg` | 23/36 lines, 10/21 branches | no |
| `fn_75_ACEC` (the head's placement) | `CSplitterMainChassis::PortPlaceHead` | 18/34 lines, 19/32 branches | no |
| `fn_75_A4E4` (the chassis's collision actor manager) | `CSplitterMainChassis::PortCreateCollisionActorManager` | 35/36 lines, 41/43 branches | no |
| `fn_75_A37C` (the health info copy) | `CSplitterMainChassis::PortCopyHealthToCollisionActors` | 8/9 lines, 3/4 branches | no |
| `fn_75_8EA4`, `fn_75_8D50` (the team exit) | `CSplitterMainChassis::PortLeaveTeam` | 2/7 lines, 1/6 branches | no |
| `fn_75_AAC4` (the player hint) | `CSplitterMainChassis::PortReleasePlayerHint` | 3/8 lines, 2/10 branches | no |
| `fn_75_9B9C` (the hit) | `CSplitterMainChassis::PortHitObject` | 9/28 lines, 5/26 branches | no |
| `fn_75_AA68` (the locomotion type) | `CSplitterMainChassis::PortSetLocomotionType` | 6/8 lines, 2/4 branches | no |
| `fn_75_E87C` (the possession with a duration) | `CSplitterMainChassis::SetIngPossessed` | 4/7 lines, 1/4 branches | the FSM lane's marker |
| `fn_75_EFA4` (the chassis's sorting bounds) | `CSplitterMainChassis::GetSortingBounds` | 3/3 lines (the shell) | no |
| `fn_75_ECA4` (the chassis's collision) | `CSplitterMainChassis::CollidedWith` | 4/21 lines, 1/26 branches | no |
| `fn_75_2154` (the head's move) | `CSplitterCommandModule::PortMoveTo` | 0/2 lines | no |
| `fn_75_2994` (the head turns upright) | `CSplitterCommandModule::PortTurnUpright` | 0/12 lines | no |
| `fn_75_9AF8` (the collision vulnerability) | `CSplitterMainChassis::PortSetCollisionVulnerability` | 0/6 lines | no |
| `fn_75_E788`, `fn_75_E768` (the dock reservation) | `CSplitterMainChassis::PortReserveDock`, `PortReleaseDock` | 0/16 and 0/2 lines | no |

The bodies that ran but whose arms did not all run are the queue's next work: the profile
names the exact lines and branches each one still misses, so the lane can take them one at a
time (`gdb` forces what no run reaches).

### 3.4 The two DoUserAnimEvent bodies (fn_75_6394, fn_75_EFA4) and fn_75_1CA0 - all three marked

**How the bodies are reached.** The override is a member of the class, so the REL's vtable slot
0x70 runs it: `CSplitterCommandModule::DoUserAnimEvent` (declared in the helpers block of
`CSplitterCommandModule.hpp`) calls `CSplitterCommandModule::PortDoUserAnimEvent`, and the
override is what the engine's vtable calls - so the body behind the marker runs whenever the
head's animation plays a user event. The same for the chassis:
`CSplitterMainChassis::DoUserAnimEvent` -> `CSplitterMainChassis::PortDoUserAnimEvent`.

**fn_75_1CA0 is ported for real** (the head's projectile aim): the alive gate (the base's flag
byte at 0x420, its 0x40 bit), the entity at 0xEFE, the node's locator transform, the entity's
own aim position (its vtable slot 0x54) and the look-at, then the launch (the transform, the
manager, 6 max projectiles, 0 attributes, no homing, the impact visor effect and the scale
1.0, 1.0, 1.0 - the rodata constant lbl_75_rodata_34). Every offset is cited in the body.

**All three are marked.** The forces were wrong before: the host is x86-64, so the event's type
is the fourth integer argument and lives in **ecx** (not in r6), and the members the arms read
are written through `$this` at the body's entry (a member is read after the entry, unlike an
argument the caller copies). The force spec is `tools/cov/specs/splitter_helpers_force.py`; one
run per arm, the counters add up (tools/cov/gcov_report.py takes several prefix dirs):

| body | arms | the force |
|---|---|---|
| `fn_75_6394` | the break-lock event with the head's flag byte clear and the data's flag word clear, the player at 0xEFE present or missing, the projectile event, another event type | `6394LOCK`, `6394FLAG`, `6394DATA`, `6394NOPLAYER`, `6394P0`, `6394P0NP`, `6394DEAD`, `6394OTHER` |
| `fn_75_1CA0` | the head dead, no player at 0xEFE, the launch | `6394P0NP`, `6394DEAD` (with the event's type in ecx, so the aim is reached) |
| `fn_75_EFA4` | the break-lock event with the chassis's flag byte clear and the player at 0xCC0 present or missing, the material event with the flag byte's 0x80 bit set or clear, another event type | `EFA4LOCK`, `EFA4FLAG`, `EFA4NOPLAYER`, `EFA4MAT`, `EFA4MATCLEAR`, `EFA4OTHER` |

The coverage build's merged profile of those runs says:

| body | lines | branches |
|---|---|---|
| `fn_75_6394` | 13/13 | 10/10, ALL-ARMS |
| `fn_75_1CA0` | 15/15 | 5/5, ALL-ARMS |
| `fn_75_EFA4` | 17/17 | 10/10, ALL-ARMS |

The chassis's animation does play a user event in the fight (its break-lock event), so its body
runs on its own; the material event (the type 0x5) is the one the fight never plays.

### 3.5 The head's DoUserAnimEvent (fn_75_6394) - the slot, the mapping and the port

`fn_75_6394` is the head's **DoUserAnimEvent** (vtable slot 0x70), not its GetSortingBounds:
the REL's own vtable (`lbl_75_data_448`) has the DOL's
`GetSortingBounds__6CActorCFRC13CStateManager` at slot **0x6C** and the module's
`fn_75_6394` at slot **0x70**, and `docs/module-analysis/vtable-offsets.md` names the DOL's
slot 0x70 `DoUserAnimEvent(CStateManager&, const CInt32POINode&, EUserEventType, float)` -
which is exactly `fn_75_6394`'s signature (r4 the manager, r5 the node, r6 the type, f1 the
dt). The chassis's slot 0x70 is `fn_75_EFA4` (the same shape: r31 = r6 the type, f31 = f1 the
dt), so **both** classes' "GetSortingBounds" stand-ins in the port are misnamed: the retail
overrides DoUserAnimEvent, not GetSortingBounds, and GetSortingBounds is inherited.

The head's override is now in the port (`CSplitterCommandModule::DoUserAnimEvent`, declared
in the helpers block of `CSplitterCommandModule.hpp`, the body in
`PortCSplitterHelpers.cpp`): the type 0x16 (kUE_BreakLockOn) case asks the player for an
orbit request and the type 0 (kUE_Projectile) case calls the head's own `fn_75_1CA0`, and the
base class's own DoUserAnimEvent runs after a case was taken (asm 0x644C-0x6468).

The **host's `CPlayer::SetOrbitRequestForTarget` takes one argument fewer** than the
retail's: the retail's mangled name is
`SetOrbitRequestForTarget__7CPlayerF9TUniqueIdQ27CPlayer19EPlayerOrbitRequestR13CStateManager`
(the target, a `CPlayer*` by value, the request and the manager) and the host's is
`SetOrbitRequestForTarget(TUniqueId, EPlayerOrbitRequest, CStateManager&)` - the `CPlayer*`
(the mangled name's `Q27CPlayer`) has no slot, so the host drops it and the player itself is
the one that is asked. The retail's call is at asm 0x6434 (the request 8 =
`kOR_ActivateOrbitSource` at asm 0x6428).

`fn_75_1CA0` (the head's projectile aim, 0x1CA0-0x1E37) is **not ported yet**: the body is
the alive gate, the player at 0xEFE, the node's locator transform, the head's own
GetAimPosition (the vtable slot 0x54) and the look-at that follows - the port has a
once-logging shell for it (`PortHeadProjectileAim`), so `fn_75_6394` stays **unmarked** (its
kUE_Projectile arm is a shell) and `fn_75_1CA0` has no body to mark. The coverage build's
profile of the run (04_cliff_contraption, the area that plays the head's anim events) says:
`fn_75_6394`'s override 3/3 lines, the body 5/13 lines and 2/10 branches - the kUE_BreakLockOn
case ran, the kUE_Projectile case did not.

### 3.6 The queue's next step: the chassis's DoUserAnimEvent

The queue rows whose retail caller ran in the coverage runs are four, and three of them are
host copies (the loader's own helpers `fn_75_E60` and `fn_75_F00`, and the destructor
`fn_75_14CC` - all class `H`). The fourth is `fn_75_1CA0` (0x198), which `fn_75_6394` calls.

`fn_75_6394` itself is the head's **DoUserAnimEvent** (vtable slot 0x70), not its
GetSortingBounds: the note's section f row ("0x70 | `GetSortingBounds(const CStateManager&)
const` | `fn_75_6394`") is off by one slot. The REL's own vtable (`lbl_75_data_448`) has the
DOL's `GetSortingBounds__6CActorCFRC13CStateManager` at slot **0x6C** and the module's
`fn_75_6394` at slot **0x70**, and `docs/module-analysis/vtable-offsets.md` names the DOL's
slot 0x70 `DoUserAnimEvent(CStateManager&, const CInt32POINode&, EUserEventType, float)` -
which is exactly `fn_75_6394`'s signature (r4 the manager, r5 the node, r6 the type, f1 the dt).
The body switches on the type (asm 0x63B4 `cmpwi r31, 0x16`): the type 0x16 asks the player
(the id at 0xEFE) for an orbit request, the type 0 calls `fn_75_1CA0`, and either way the
base class's own `DoUserAnimEvent` runs (asm 0x6468).

The next one is the chassis's own `fn_75_EFA4` (the chassis's DoUserAnimEvent, the same
slot): its body switches on the type 0x16 and the type 0x5 (asm 0xEFC0, 0xEFDC). It is ported
and marked now (section 3.4): the port's chassis has a DoUserAnimEvent override that calls
`CSplitterMainChassis::PortDoUserAnimEvent`.

### 3.7 The parked lane's eight open bodies: what each untaken arm reads

The eight bodies the parked Splitter lane left open (32F4, 1188, 23F0, 270C, 909C, 8EC8, D61C,
E87C) were checked one by one. For each, the value the untaken arm reads decides how it can be
forced: a member is written through `$this` at the body's entry, an argument is written in its
register (only where the disassembly shows it is not copied before the use), and a callee's
result can only be taken by breaking on the callee.

| body | what the untaken arm reads | how it is forced | the profile |
|---|---|---|---|
| `fn_75_32F4` (the head's UpdateFlashAndDeath) | the flash weight at 0xF08, the flag byte's 0x04 bit at 0xF6A and the death timer at 0xEF0 (all members), the alive bit at 0x420 (the base's flag) | the members through `$this` (`32F4`, `32F4D`, `32F4DEAD`, `32F4F`) | 17/17 lines, 10/10 branches, **marked** |
| `fn_75_1188` (the head's SpawnBeamEffect) | the base's detection range at 0x3D0 (a member) | the member through `$this` (`1188`) | 36/36 lines, 19/21 branches, **marked** - the two untaken branches are the `new`'s allocation-failure path and an internal path of the inlined AddObject, neither a live arm |
| `fn_75_23F0` (the head's PortSeparationVector) | the listening-AI list's objects: a null cast and an AI in another area (the world state, not a member) | not forceable: it needs another actor in the list that is not a CPatterned | 0/19 lines: the body is never called in this fight |
| `fn_75_270C` (the head's PortDodgeDirection) | the same list (the null and the other-area arms) and the random bit for the both-sides-open pick | the list arms are the world state | 0/27 lines: the head never dodges in this fight |
| `fn_75_909C` (the chassis's PortDodgeIsClear) | the ray's result and the path search's (callee results) | the body is never called | 0/7 lines |
| `fn_75_8EC8` (the chassis's PortSeparationVector) | the listening-AI list's objects: a null cast (the list holds CPatterneds, so it never comes up) and an AI in another area | the null cast is the world state; the other-area arm needs another actor's area id | 20/20 lines, 20/23 branches - not marked |
| `fn_75_D61C` (the chassis's Dodge state) | the dodge direction at 0xD04 and the head's id at 0xC6C (members) | the members through `$this` at the state's entry | 0/21 lines: the Dodge state is never entered in this fight (the force's breakpoint never hits) |
| `fn_75_E87C` (the chassis's SetIngPossessed with the duration) | the possession flag (an argument) and the command module's id at 0xC6C (a member) | the flag in esi (the second argument) and the id through `$this` (`E87C`) | 7/7 lines, 4/4 branches, **marked** - the arm is a race: the chassis's command module id is 0xFFFF (no docked head), so the force writes the head u0531's id 0x531, and the arm needs the head to be in the manager when the possession runs. The run that caught it says 7/7 lines and 4/4 branches; a later run of the same force missed it (the head was not there yet) |

The two GetSortingBounds overrides the port had are gone as well: the retail inherits the DOL's
GetSortingBounds (vtable slot 0x6C) and the module's slot 0x70 is DoUserAnimEvent, so the
overrides - which only forwarded to `CPatterned::GetSortingBounds` behind a stand-in log - were
spurious.

## 4. What is left, and why

- **The queue's rows with a body but no marker** are the ones the coverage build shows the
  shipped spawns reach: section 3.3 lists them with the lines and the branches each one still
  misses. The next ones are `fn_75_2CE0` (the head's shield: 5/34 lines), `fn_75_8D50` (the
  team exit: 2/7) and `fn_75_AAC4` (the player hint: 3/8) - the smallest gaps.
- **The queue's rows with no body at all** are the module's own helpers that the FSM lane's
  states and codes call (the beam effect's actor, the laser-pulse shot, the collision actor
  managers' joint helpers) and the ones the two classes' constructors build. They are listed in
  section 2 with their callers; the FSM lane can wire them one at a time.
- **Four rows need a host type the port does not have yet:** the beam effect's own class (the
  port has a `CSplitterBeamEffect` shell), the laser-pulse shot's class and the collision actor
  manager's joint descriptions.
- **The `.ctors` initialisers** `fn_75_7C80` and `fn_75_FED0` (the head's and the chassis's
  table builders) are B rows with no marker: the port's own FSM tables are their result, so
  they can be marked the way the MinorIng module's `fn_44_ABCC` is (on the port's tables) once
  the FSM lane's tables are final.

## 5. The port's new file

`src/MetroidPrime/Enemies/PortCSplitterHelpers.cpp` holds the bodies; it is listed in
`tools/modules_port.txt` and `files.cmake` was regenerated with `python3 tools/gen_files_cmake.py`.

The lane's own tools: `tools/drivers/minoring_scan.py`-style scans are not in this branch; the
classification above was made with the same three scans the note's section 1 names (the direct
calls, the table references and the port's state), and the coverage build's profile
(`tools/cov/gcov_report.py`) says which bodies the shipped spawns reach.
