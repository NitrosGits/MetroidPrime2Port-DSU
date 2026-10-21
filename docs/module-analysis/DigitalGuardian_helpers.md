# DigitalGuardian REL (module 14): the stateless helpers (this lane's half)

This note is the helpers lane's half of the Digital Guardian module: the module's **B-class
functions that are not FSM handlers** - the math, target/aim, collider/vulnerability and
effect/sound helpers the body's and the head's state machines call. The module's own note is
`docs/module-analysis/DigitalGuardian.md`; this note does not edit it, and it does not edit the
FSM tables (the states, the triggers and the codes belong to the Opus lane on
`decomp/digitalguardian`).

The ported bodies live in `src/MetroidPrime/Enemies/PortCDigitalGuardianHelpers.cpp`; the
declarations are in one append-only `#ifdef TARGET_PC` block at the end of the class in
`include/MetroidPrime/Enemies/CDigitalGuardian.hpp` (the body's nine) and in the same kind of
block at the end of the class in `include/MetroidPrime/Enemies/CDigitalGuardianHead.hpp` (the
head's five). The head's block is a deviation from the brief, which named only the body's
header: the head's helpers are members of `CDigitalGuardianHead`, so they cannot be declared in
the body's header.

## 1. How each row was classified

Three scans, all over the module's own build in the archive tree
(`<archive>/build/G2ME01/DigitalGuardian/asm`):

1. **Direct calls.** `bl fn_14_XXXX` over the module's `.text`: the retail call sites.
2. **Table references.** The module's `.data` (the vtables and the three FSM registration
   tables). The FSM rows are the ones the module's static initialisers (`fn_14_D904` for the
   body, `fn_14_1AD2C` for the head) put into `lbl_14_data_9C/_204/_380` and
   `lbl_14_data_808/_A14/_C80`.
3. **The port's state.** What the tree already has for each row: a body (a `Port*.cpp` def whose
   comment cites the offset), a stand-in, a declaration only, or nothing.

The **kind** column uses the Sandworm note's rule set (`docs/module-analysis/Sandworm.md`
section g), with one extra class for this module:

| class | meaning | the port |
|---|---|---|
| **F** | an FSM handler (a state, a trigger or a code of one of the six tables) | the Opus lane's |
| **H** | a host copy: the body is a copy of something the host already has (the loaders and their `SLdr` helpers, the compiler-emitted destructors, the `rstl`/template code, the DOL class members copied inline) | no body is written |
| **T** | a data or vtable thunk: the body only returns a constant, a member address or a flag | one line in the vtable |
| **B** | module behaviour: real logic | a port body - this lane's half |

The counts (all 420 symbols of `config/G2ME01/rels/DigitalGuardian/symbols.txt`):

| class | functions | .text bytes |
|---|---|---|
| F FSM handler | 106 | 0x5C94 (23,700) |
| H host copy | 97 | 0x6154 (24,916) |
| T data / vtable thunk | 25 | 0x35C (860) |
| B module behaviour | 192 | 0xF758 (63,320) |
| **total** | **420** | **0x1B89C (112,796)** |

Of the 192 B rows, 105 are the FSM handlers' own helpers that the other lane's states call and
that this lane did not touch (they are listed in the table below with their kind **B**), and 87
are the module's own stateless helpers. This lane ports 15 of them (section 3).

## 2. The rows


## 3. The helpers this lane ports

All 15 bodies are in `src/MetroidPrime/Enemies/PortCDigitalGuardianHelpers.cpp`; the declarations
are in the append-only helpers block at the end of the class in
`include/MetroidPrime/Enemies/CDigitalGuardian.hpp` (the body's ten) and in the same kind of
block at the end of the class in `include/MetroidPrime/Enemies/CDigitalGuardianHead.hpp` (the
head's five). The head's block is a deviation from the brief, which named only the body's
header: the head's helpers are members of `CDigitalGuardianHead`, so they cannot be declared in
the body's header.

Every bit test, mask, compare and constant carries the retail asm offset it came from.

| id | size | what | the class | marked |
|---|---|---|---|---|
| `fn_14_37FC` | 0xC0 | the entry of the five-value table {.rodata 0x24C} nearest to the argument | body | no |
| `fn_14_6C74` | 0x3C | the attack angle (4*pi, pi/3 or 2*pi/3) | body | **yes** |
| `fn_14_3F90` | 0xF0 | the four knee armours follow the knee locators | body | **yes** |
| `fn_14_38BC` | 0xB8 | the leg-stab, jump and protect-the-knees timers | body | **yes** |
| `fn_14_2358` | 0x18C | the toe targets' sound emitters and hit timers | body | **yes** |
| `fn_14_5644` | 0x1F4 | the "XRDG" case: a toe target hit by the boost ball | body | **yes** |
| `fn_14_6C24` | 0x50 | is the head's turn direction unusable? | body | **yes** |
| `fn_14_6CB0` | 0x154 | the aim: the forward and the up vector | body | **yes** |
| `fn_14_6E04` | 0x258 | the body turns to the head module | body | no |
| `fn_14_83C0` | 0x184 | the knock-back animation parm data | body | no |
| `fn_14_10120` | 0xC0 | the head's projectile-attack block picker | head | no |
| `fn_14_F374` | 0x1C | the head's retreat speed | head | no |
| `fn_14_11EA8` | 0x88 | the destroyed bomb slots' sounds are stopped | head | **yes** |
| `fn_14_11F7C` | 0x70 | the echo targets are deleted | head | **yes** |
| `fn_14_137A8` | 0x88 | the head's armour model | head | no |

Four call sites now call them instead of logging: the body's "XRDG" case
(`PortCDigitalGuardian.cpp`, the `StandInFn("fn_14_5644 ...")` line), the head's "XDEL" case
(`PortCDigitalGuardianHead.cpp`, the two `StandInFn` lines for `fn_14_11EA8` and `fn_14_11F7C`),
the body's Think (`fn_14_A5BC`, which the retail ends with the four calls for `fn_14_6E04`,
`fn_14_2358`, `fn_14_38BC` and `fn_14_3F90`; the port's Think had them missing) and the head's
constructor (`fn_14_18264`, which the retail ends with the call for `fn_14_137A8` at 0x18E60; the
port's constructor had it missing).

### 3.1 The runs

**Sanctuary Fortress `09_cliff_temple_dark`** (world 1baa96c2, area 53), layers 3 and 11, the
guardian body `u054e` and the head module `u051b` woken with `send <id> Activate`, the body
sent the "XRDG" message (`send u054e ResistedDamage`, the console's name for 0x58524447) and
the head deleted with `send u051b Delete` at frame 2400. `MP2_FRAME_LIMIT=0
MP2_TICKS_PER_FRAME=8`, 3000 frames.

Two runs, both with the game exiting with code 0 and the log free of `[port] caught` and
`reach-stub` lines:

    # the plain coverage run
    tools/cov/run.sh tools/cov/specs/digitalguardian.py /tmp/opencode/dg_cov4 \
      1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_area53.con --fast --frames 3000

    # the coverage run with the forcing (the arms the fight cannot reach)
    COV_GDB_SCRIPT=tools/cov/force_cov.py tools/cov/force.sh \
      tools/cov/specs/digitalguardian_force.py /tmp/opencode/dg_force6 \
      1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_area53.con --fast --frames 3000

The forcing side is `tools/cov/force.sh` and `tools/cov/force.py`, **copied from the
`decomp/darksamus-helpers2` branch** (the DarkSamus helpers lane wrote them); `tools/cov/force_cov.py`
is this lane's combination of them with `tools/cov/gdb_cov.py`, so one run gives both the arm
counts (`cov.txt`) and the forced writes (`forced.log`, `forced.txt`). The forcing writes the
members listed in `tools/cov/specs/digitalguardian_force.py` at the entry of a body; the spec's
`FORCE` entries say which member and why.

### 3.2 The arms

The "forced" arms are the ones the fight cannot produce: the write happens at the entry of the
body, so the arm is taken with a member the fight never sets.

| id | the arms | how they ran |
|---|---|---|
| `fn_14_3F90` (the knee armours follow the knees) | (a) the knee's id is invalid: the walk skips it, (b) the actor the id names is gone: the walk clears the id, (c) the armour is there: its transform is set | (c) **naturally** (the first 101 frames, both ids valid, the `CActor::SetTransform` call site); (a) **forced** (the second knee id cleared at the entry, the hits 101-200, the key "65535, 1402"); (b) **forced** (the third knee id pointed at an actor that does not exist, the hits 201+, the key "65535, 65535" - the body cleared the id itself) |
| `fn_14_38BC` (the timers) | (a) four knees left: the protect timer counts down, (b) fewer knees and the player is morphed: the timer is reset to the data block's +0x170, (c) fewer knees and the player is not morphed: the timer counts down | (a) **naturally** (the first 101 frames, the key "4, 0"); (c) **naturally** with the knee count forced under four (the second knee id cleared at the entry, the hits 101-300, the keys "3, 0" and "2, 0"); (b) **forced** (the player's morph state set to 1 at the entry, the hits 301+, the key "2, 1") |
| `fn_14_6C74` (the attack angle) | (a) the vortex mode is not 0: 4*pi, (b) the flag byte's bit 6 is set: pi/3, (c) neither: 2*pi/3 | (c) **naturally** (the first 101 frames, the key "0, 0"); (a) **forced** (the vortex mode set to 1 at the entry, the hits 101-200, the key "1, 0"); (b) **forced** (the flag byte's bit 6 set at the entry, the hits 201+, the key "0, 64") |
| `fn_14_6C24` (the head's turn direction) | (a) the guardian's body state is 0: the direction does not matter, (b) the head's turn direction is not magnitude-safe: the direction is unusable, (c) neither: the direction is usable | (c) **naturally** (the frames where the head's direction was safe, `x6CB0`'s `CTransform4f::operator*` call site); (b) **naturally** (the frames where it was not, `x6CB0`'s `CVector3f::AsNormalized` call site); (a) **forced** (the body controller's state id set to 0 at the entry, the hits 101+; the `CVector3f::IsMagnitudeSafe` call site then stopped: 203 entries, 100 calls) |
| `fn_14_6CB0` (the aim) | (a) the flag byte's bit 1 is clear and the head has no usable direction: the aim transform is rebuilt, (b) the direction is usable: the aim is the given transform times the aim transform, (c) it is not: the direction is the head's normalized turn direction | (b) **naturally** (the `CTransform4f::operator*` call site, 102 hits); (c) **naturally** (the `CVector3f::AsNormalized` call site, 101 hits); (a) **forced** (the flag byte's bit 1 cleared at the entry, the hits 101+; then the `fn_14_13B38`, `CTransform4f::TransposeRotate` and `CTransform4f::LookAt` call sites fired, 52 hits each) |
| `fn_14_2358` (the toe targets' sound emitters) | (a) the toe target has a sound emitter: it follows the toe locator, (b) the toe target is flinching: it goes back to the target state | (a) **naturally** (every frame, the `CSfxManager::UpdateEmitter` call site, 400 hits); (b) **naturally** and **forced** (the `fn_14_1A68` call site, 400 hits; the flinch was forced by setting the first toe target's state word to 2 at the entry) |
| `fn_14_11EA8` (the bomb slots' sounds) | (a) the slot has a sound: the emitter is removed and the handle cleared, (b) the slot has no sound: nothing happens | (b) **naturally** in the plain run (the head's "XDEL" reached the body and no bomb slot had a sound: the entry was hit once, the `CSfxManager::RemoveEmitter` call site not at all); (a) **forced** (a sound handle written into the first bomb slot at the entry; the `CSfxManager::RemoveEmitter` call site then fired) |
| `fn_14_11F7C` (the echo targets are deleted) | (a) the walk deletes an echo target and clears its id, (b) the walk ends when the count is reached | (a) and (b) **naturally** (the entry was hit once and the `CStateManager::DeleteObjectRequest` call site three times: the three echo targets were deleted and the walk ended) |
| `fn_14_5644` (the "XRDG" case) | nine early-outs and two endings | the port's "XRDG" case calls it and the console's "ResistedDamage" message reached it (the entry was hit once, the `CStateManager::ObjectById` call site once), but the sender is not a toe target collider, so only the first early-out ran. The toe target colliders (guest this+0x1078) do not exist in the port, so the deeper arms cannot run. **Unmarked** |
| `fn_14_6E04` (the body turns to the head) | (a) no head, (b) the head is gone: it detaches, (c) the angle is below the turn rate, (d) else: the head's forward is slerped, (e) at most two knees left and the head is attacking: the flag byte's bit 1 is set | (a) **forced** (the head's id cleared at the entry, the hits 101-200), (b) **forced** (the head's id pointed at an actor that does not exist, the hits 201+), (c) and (d) **naturally** (the `CTransform4f::LookAt` and `CVector3f::Slerp` call sites). (e) needs the head's projectile attack (head+0x10D0) to be 1, which only the head's FSM code (the other lane's) sets. **Unmarked** |
| `fn_14_37FC` (the nearest 45 degrees) | five compares, the earliest wins a tie | the port has no caller yet (`fn_14_7208` and `fn_14_77EC`, the FSM codes, are still stand-ins). **Unmarked** |
| `fn_14_83C0` (the knock-back parm data) | none: the body fills the struct field by field | the port's `fn_14_8090` body (KneeDestroyedReaction, the FSM lane's, already marked) builds the same data inline with the host's constructor. **Unmarked** |
| `fn_14_10120` (the projectile-attack block) | six (the knee count 0..4 and the default, and the flag byte's bit 4 in the 0 case) | the port has no caller yet (the head's codes call it). **Unmarked** |
| `fn_14_F374` (the retreat speed) | two (more than one life left, one life left) | the port has no caller yet (`fn_14_1569C`, FollowRetreatPath, is still a stand-in). **Unmarked** |
| `fn_14_137A8` (the armour model) | two (the armour id is 0xFFFE: nothing, else the model is built) | the port has no caller yet (the head's constructor calls it). **Unmarked** |

### 3.3 `fn_14_83C0`, the knock-back struct, field by field

The retail's function fills the animation parm data of the knee's knock-back. The struct it
writes is `{u32 stateId; u32 parmCount; CPASAnimParm parms[8]}`:

| field | retail | the host |
|---|---|---|
| the state id (the first word) | the caller's `li r4, 0x6` (0x81D0) = `pas::kAS_KnockBack`, stored at 0x83F0 | `CPASAnimParmData::mStateId` (the same enum) |
| the parameter count (the second word) | the size the callee counts up from 0 (the store at 0x83FC) to 8 (the eight stores at 0x8414, 0x8448, 0x846C, 0x848C, 0x84A8, 0x84C8, 0x84EC and 0x8514) | `rstl::reserved_vector< CPASAnimParm, 8 >::mCount` (the host's reserved_vector is `{int mCount; uchar mData[N * sizeof(T)];}`, so the count sits where the retail's had it) |
| the eight parameters (eight pairs of words, a `CPASAnimParm` is 8 bytes: the value and the type) | written in the order p1..p8: the knee's angle first (`.rodata 0x20C`, the `FromReal32` call at 0x81A0), then the enum 2 (the `FromEnum` call at 0x8184), then the six `NoParameter` ones (the `NoParameter` calls at 0x8150, 0x8158, 0x8160, 0x8168, 0x8170 and 0x8178) | `rstl::reserved_vector< CPASAnimParm, 8 >::mData` (the same eight parameters in the same order) |

**No mismatch to report**: the host's `CPASAnimParmData` has the same layout as the retail's
(the state id, the count, then the eight parameters), so the body is the host's constructor.
The one thing that is *not* the same is the caller: the retail's `fn_14_8090` passes the eight
parameters as pointers (in reverse stack order) and the count as a separate argument, while the
port's `KneeDestroyedReaction` (the FSM lane's, already marked) builds the same data inline with
`CPASAnimParmData(pas::kAS_KnockBack, CPASAnimParm::FromReal32(angle),
CPASAnimParm::FromEnum(2))` - the host's constructor fills the six missing parameters with
`NoParameter()`, exactly as the retail's six `NoParameter` calls do.

## 4. What is left, and why

- **The helpers with no caller on port** (the table in section 5): `fn_14_37FC`, `fn_14_10120`,
  `fn_14_F374` and `fn_14_137A8` - the FSM lane's codes and states and the head's constructor
  call them.
- **Two rows need the head's state, which the port does not produce:** `fn_14_6E04`'s last arm
  (the head's projectile attack) and `fn_14_5644`'s arms (the toe target colliders, guest
  this+0x1078, do not exist in the port).
- **Four rows need a host type the port does not have yet:**
  - `fn_14_476C` (the `SLdrShockWaveInfo` -> `CShockWaveInfo` conversion): the host has no
    `CShockWave` (the vortex, `fn_14_458C`, is the FSM lane's).
  - `fn_14_3BFC` (the `SLdrDamageVulnerability` -> `CDamageVulnerability` conversion): the host
    has `LdrToDamageVulnerability` (ScriptLoader.hpp line 57), so this is a host copy in
    principle; the module's own copy keeps its own allocation.
  - `fn_14_6B5C` (the `CToken` copy): the host's `CToken` has the same copy.
  - `fn_14_1570` (the model's bounding box for the scan visor): the host's `CAABox` has
    `GetTransformedAABox`, but the retail reads the model's own max point (the guest's
    this+0x54), which the host's `CModelData` does not expose.
- **The module's other classes' constructors and destructors** (`fn_14_11BA8`, `fn_14_13A80`,
  `fn_14_12FE0`, `fn_14_13A74` and the `CValidEntityPredicate` chain): the host does not have
  those classes.
- **One row is the "XHIT" case, `fn_14_4F2C`** (0x214): it builds a `CDamageInfo` from the
  module's static table (`.data 0x8`, the word 0x3B) through the retail's `__shl2i` helper, and
  that helper's meaning is not known yet. It is left to the Opus DigitalGuardian lane.

## 5. The helpers the FSM lane's states call (the port order)

The states and codes that are still stand-ins in the port call these of this half. They are
listed so the lane can write the states against them (the bodies are in the port now).

| helper | size | called from | the port |
|---|---|---|---|
| `fn_14_37FC` | 0xC0 | `fn_14_7208` (SetJumpFaceTarget) and `fn_14_77EC` (SetProtectKneesFaceTarget) | **ported**, unmarked |
| `fn_14_6C74` | 0x3C | `fn_14_6E04` | **ported, marked** |
| `fn_14_3F90` | 0xF0 | `fn_14_A5BC` (the Think) | **ported, marked** |
| `fn_14_38BC` | 0xB8 | `fn_14_A5BC` (the Think) | **ported, marked** |
| `fn_14_2358` | 0x18C | `fn_14_A5BC` (the Think) | **ported, marked** |
| `fn_14_5644` | 0x1F4 | `fn_14_A6A8` (the "XRDG" case) | **ported**, unmarked |
| `fn_14_6C24` | 0x50 | `fn_14_6CB0` | **ported, marked** |
| `fn_14_6CB0` | 0x154 | `fn_14_6E04` | **ported, marked** |
| `fn_14_6E04` | 0x258 | `fn_14_A5BC` (the Think) | **ported**, unmarked |
| `fn_14_83C0` | 0x184 | `fn_14_8090` (KneeDestroyedReaction) | **ported**, unmarked |
| `fn_14_10120` | 0xC0 | `fn_14_15448` (SelectProjectileAttack), `fn_14_15598` (ResetProjectileAttackTimes), `fn_14_16F74` (ProjectileTelegraph) | **ported**, unmarked |
| `fn_14_F374` | 0x1C | `fn_14_1569C` (FollowRetreatPath) | **ported**, unmarked |
| `fn_14_11EA8` | 0x88 | `fn_14_17D38` (the head's "XDEL" case) | **ported, marked** |
| `fn_14_11F7C` | 0x70 | `fn_14_17D38` (the head's "XDEL" case) | **ported, marked** |
| `fn_14_137A8` | 0x88 | `fn_14_18264` (the head's constructor) | **ported**, unmarked |

## 6. The port's new file

`src/MetroidPrime/Enemies/PortCDigitalGuardianHelpers.cpp` holds the bodies; it is listed in
`tools/modules_port.txt` and `files.cmake` was regenerated with `python3 tools/gen_files_cmake.py`.
| id | size | kind | callers | the port |
|---|---|---|---|---|
| `fn_14_0` | 0x8 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_8` | 0x8 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_10` | 0x68 | T | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_78` | 0x10 | T | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_88` | 0x8 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_90` | 0x8 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_98` | 0x8 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_A0` | 0x8 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_A8` | 0x8 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_B0` | 0x10 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_C0` | 0xC | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_CC` | 0xC | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_D8` | 0x8 | T | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_E0` | 0x8 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_E8` | 0x8 | T | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_F0` | 0x1C | T | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_10C` | 0x2C | T | 0: - | body (PortCDigitalGuardian.cpp) |
| `RELExit` | 0x24 | T | 0: - | no body |
| `RELMain` | 0x20 | T | 0: - | no body |
| `fn_14_17C` | 0x3C | T | 1: RELMain | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_1B8` | 0x330 | H | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_4E8` | 0x40 | H | 2: fn_14_1B8, fn_14_DF7C | no body |
| `fn_14_528` | 0x20 | H | 2: fn_14_4E8, fn_14_6A50 | body (PortCDigitalGuardian.cpp) |
| `fn_14_548` | 0x28 | H | 1: fn_14_528 | no body |
| `fn_14_570` | 0x13C | H | 1: fn_14_548 | no body |
| `fn_14_6AC` | 0x6C | H | 0: - | no body |
| `fn_14_718` | 0x54 | H | 0: - | no body |
| `fn_14_76C` | 0x64 | H | 0: - | no body |
| `fn_14_7D0` | 0x60 | H | 0: - | no body |
| `fn_14_830` | 0x7C | H | 0: - | no body |
| `fn_14_8AC` | 0x82C | H | 1: fn_14_1B8 | no body |
| `fn_14_10D8` | 0x148 | H | 3: fn_14_1B8, fn_14_830, fn_14_AB98 | no body |
| `fn_14_1220` | 0x21C | H | 1: fn_14_1B8 | no body |
| `fn_14_143C` | 0xBC | H | 3: fn_14_5140, fn_14_5470, fn_14_5644 | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_14F8` | 0x78 | H | 4: fn_14_8090, fn_14_8580, fn_14_8E44, fn_14_A6A8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_1570` | 0x190 | H | 1: fn_14_9CA4 | no body |
| `fn_14_1700` | 0xDC | H | 2: fn_14_9ABC, fn_14_A004 | body (PortCDigitalGuardian.cpp) |
| `fn_14_17DC` | 0xD8 | H | 1: fn_14_A004 | body (PortCDigitalGuardian.cpp) |
| `fn_14_18B4` | 0xE0 | H | 2: fn_14_9ABC, fn_14_A004 | body (PortCDigitalGuardian.cpp) |
| `fn_14_1994` | 0xD4 | H | 2: fn_14_9ABC, fn_14_A004 | body (PortCDigitalGuardian.cpp) |
| `fn_14_1A68` | 0x580 | B | 9: fn_14_2358, fn_14_24E4, fn_14_458C, fn_14_5140, fn_14_5644, fn_14_7D8C | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_1FE8` | 0x58 | H | 10: fn_14_11FEC, fn_14_12314, fn_14_12B04, fn_14_15054, fn_14_1A68, fn_14_ | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_2040` | 0x54 | H | 1: fn_14_1FE8 | no body |
| `fn_14_2094` | 0x50 | H | 0: - | no body |
| `fn_14_20E4` | 0xBC | B | 1: fn_14_458C | no body |
| `fn_14_21A0` | 0x28 | B | 6: fn_14_13284, fn_14_20E4, fn_14_4464, fn_14_44F8, fn_14_59BC, fn_14_61D | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHead.cpp) |
| `fn_14_21C8` | 0x7C | B | 1: fn_14_21A0 | no body |
| `fn_14_2244` | 0x114 | B | 1: fn_14_21C8 | no body |
| `fn_14_2358` | 0x18C | B | 1: fn_14_A5BC | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_24E4` | 0x94 | B | 2: fn_14_8090, fn_14_A6A8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_2578` | 0xF4 | B | 4: fn_14_17DC, fn_14_9ABC, fn_14_A138, fn_14_B100 | body (PortCDigitalGuardian.cpp) |
| `fn_14_266C` | 0x264 | B | 1: fn_14_5274 | body (PortCDigitalGuardian.cpp) |
| `fn_14_28D0` | 0x158 | B | 1: fn_14_2CC4 | body (PortCDigitalGuardian.cpp) |
| `fn_14_2A28` | 0x29C | B | 1: fn_14_2CC4 | body (PortCDigitalGuardian.cpp) |
| `fn_14_2CC4` | 0x19C | B | 1: fn_14_7CB4 | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_2E60` | 0x22C | B | 1: fn_14_5274 | body (PortCDigitalGuardian.cpp) |
| `fn_14_308C` | 0x120 | B | 3: fn_14_7208, fn_14_7600, fn_14_9678 | body (PortCDigitalGuardian.cpp) |
| `fn_14_31AC` | 0x100 | B | 2: fn_14_119E4, fn_14_308C | body (PortCDigitalGuardian.cpp) |
| `fn_14_32AC` | 0x54 | H | 4: fn_14_119E4, fn_14_13830, fn_14_17F64, fn_14_308C | body (PortCDigitalGuardian.cpp) |
| `fn_14_3300` | 0x1E8 | B | 1: fn_14_A468 | no body |
| `fn_14_34E8` | 0x18C | B | 1: fn_14_9070 | body (PortCDigitalGuardian.cpp) |
| `fn_14_3674` | 0x188 | B | 1: fn_14_9260 | body (PortCDigitalGuardian.cpp) |
| `fn_14_37FC` | 0xC0 | B | 2: fn_14_7208, fn_14_77EC | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_38BC` | 0xB8 | B | 1: fn_14_A5BC | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_3974` | 0x58 | B | 2: fn_14_676C, fn_14_9260 | body (PortCDigitalGuardian.cpp) |
| `fn_14_39CC` | 0x78 | B | 1: fn_14_9260 | body (PortCDigitalGuardian.cpp) |
| `fn_14_3A44` | 0x108 | B | 7: fn_14_76D0, fn_14_775C, fn_14_7D8C, fn_14_8090, fn_14_8580, fn_14_A468 | body (PortCDigitalGuardian.cpp) |
| `fn_14_3B4C` | 0x5C | B | 4: fn_14_18264, fn_14_1A698, fn_14_3A44, fn_14_B100 | body (PortCDigitalGuardian.cpp) |
| `fn_14_3BA8` | 0x54 | H | 0: - | no body |
| `fn_14_3BFC` | 0x130 | B | 1: fn_14_3B4C | no body |
| `fn_14_3D2C` | 0x7C | B | 6: fn_14_76D0, fn_14_775C, fn_14_7D8C, fn_14_8090, fn_14_8580, fn_14_A468 | body (PortCDigitalGuardian.cpp) |
| `fn_14_3DA8` | 0x84 | B | 2: fn_14_775C, fn_14_8E44 | body (PortCDigitalGuardian.cpp) |
| `fn_14_3E2C` | 0x5C | B | 1: fn_14_5838 | body (PortCDigitalGuardian.cpp) |
| `fn_14_3E88` | 0x8C | B | 1: fn_14_A6A8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_3F14` | 0x7C | B | 1: fn_14_A6A8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_3F90` | 0xF0 | B | 1: fn_14_A5BC | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_4080` | 0x274 | B | 1: fn_14_A6A8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_42F4` | 0x5C | B | 3: fn_14_4080, fn_14_5C30, fn_14_61DC | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_4350` | 0x5C | B | 2: fn_14_5140, fn_14_5644 | body (PortCDigitalGuardian.cpp) |
| `fn_14_43AC` | 0x5C | B | 3: fn_14_5644, fn_14_5838, fn_14_59BC | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_4408` | 0x5C | B | 1: fn_14_59BC | body (PortCDigitalGuardian.cpp) |
| `fn_14_4464` | 0x94 | B | 1: fn_14_61DC | body (PortCDigitalGuardian.cpp) |
| `fn_14_44F8` | 0x94 | B | 1: fn_14_61DC | body (PortCDigitalGuardian.cpp) |
| `fn_14_458C` | 0x1E0 | B | 1: fn_14_A468 | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_476C` | 0xCC | B | 1: fn_14_458C | no body |
| `fn_14_4838` | 0x40 | B | 1: fn_14_9260 | body (PortCDigitalGuardian.cpp) |
| `fn_14_4878` | 0x98 | B | 1: fn_14_9260 | body (PortCDigitalGuardian.cpp) |
| `fn_14_4910` | 0x70 | H | 4: fn_14_19908, fn_14_4878, fn_14_4D54, fn_14_4E00 | no body |
| `fn_14_4980` | 0x58 | H | 1: fn_14_4910 | no body |
| `fn_14_49D8` | 0x54 | H | 1: fn_14_4980 | no body |
| `fn_14_4A2C` | 0xAC | B | 3: fn_14_4878, fn_14_4D54, fn_14_4E00 | body (PortCDigitalGuardian.cpp) |
| `fn_14_4AD8` | 0xAC | B | 1: fn_14_4A2C | no body |
| `fn_14_4B84` | 0x1D0 | B | 1: fn_14_4AD8 | no body |
| `fn_14_4D54` | 0xAC | B | 1: fn_14_9260 | body (PortCDigitalGuardian.cpp) |
| `fn_14_4E00` | 0x12C | B | 1: fn_14_A468 | no body |
| `fn_14_4F2C` | 0x214 | B | 1: fn_14_A6A8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_5140` | 0x134 | B | 1: fn_14_5384 | body (PortCDigitalGuardian.cpp) |
| `fn_14_5274` | 0x110 | B | 1: fn_14_5838 | body (PortCDigitalGuardian.cpp) |
| `fn_14_5384` | 0xEC | B | 1: fn_14_5838 | body (PortCDigitalGuardian.cpp) |
| `fn_14_5470` | 0x1D4 | B | 1: fn_14_5838 | body (PortCDigitalGuardian.cpp) |
| `fn_14_5644` | 0x1F4 | B | 1: fn_14_A6A8 | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_5838` | 0xD8 | B | 1: fn_14_A6A8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_5910` | 0xAC | B | 1: fn_14_A6A8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_59BC` | 0x274 | B | 1: fn_14_676C | body (PortCDigitalGuardian.cpp) |
| `fn_14_5C30` | 0x2F4 | B | 1: fn_14_676C | body (PortCDigitalGuardian.cpp) |
| `fn_14_5F24` | 0x38 | B | 3: fn_14_13318, fn_14_5C30, fn_14_61DC | cited only (PortCDigitalGuardian.cpp, PortCDigitalGuardianHead.cpp) |
| `fn_14_5F5C` | 0x20 | B | 2: fn_14_5F24, fn_14_D89C | no body |
| `fn_14_5F7C` | 0x28 | B | 1: fn_14_5F5C | no body |
| `fn_14_5FA4` | 0xC0 | B | 4: fn_14_13318, fn_14_5C30, fn_14_5F7C, fn_14_61DC | cited only (PortCDigitalGuardian.cpp, PortCDigitalGuardianHead.cpp) |
| `fn_14_6064` | 0x5C | H | 0: - | no body |
| `fn_14_60C0` | 0x84 | H | 3: fn_14_13318, fn_14_5C30, fn_14_61DC | body (PortCDigitalGuardian.cpp) |
| `fn_14_6144` | 0x38 | B | 1: fn_14_60C0 | no body |
| `fn_14_617C` | 0x60 | B | 0: - | no body |
| `fn_14_61DC` | 0x590 | B | 1: fn_14_676C | body (PortCDigitalGuardian.cpp) |
| `fn_14_676C` | 0xF8 | B | 1: fn_14_A5BC | body (PortCDigitalGuardian.cpp) |
| `fn_14_6864` | 0x1BC | B | 1: fn_14_B100 | body (PortCDigitalGuardian.cpp) |
| `fn_14_6A20` | 0x30 | B | 2: fn_14_137A8, fn_14_6864 | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_6A50` | 0x48 | B | 1: fn_14_6A20 | body (PortCDigitalGuardian.cpp) |
| `fn_14_6A98` | 0xC4 | B | 1: fn_14_6A50 | body (PortCDigitalGuardian.cpp) |
| `fn_14_6B5C` | 0xC8 | B | 1: fn_14_6A98 | no body |
| `fn_14_6C24` | 0x50 | B | 1: fn_14_6CB0 | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_6C74` | 0x3C | B | 1: fn_14_6E04 | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_6CB0` | 0x154 | B | 1: fn_14_6E04 | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_6E04` | 0x258 | B | 1: fn_14_A5BC | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_705C` | 0x58 | B | 7: fn_14_10120, fn_14_14F8, fn_14_15448, fn_14_38BC, fn_14_6E04, fn_14_7B | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_70B4` | 0x30 | B | 1: fn_14_17244 | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHead.cpp) |
| `fn_14_70E4` | 0x84 | B | 2: fn_14_16160, fn_14_6E04 | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHead.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_7168` | 0xA0 | B | 1: fn_14_13830 | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHead.cpp) |
| `fn_14_7208` | 0x1CC | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_73D4` | 0x130 | F | 1: fn_14_A468 | body (PortCDigitalGuardian.cpp) |
| `fn_14_7504` | 0xFC | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_7600` | 0x48 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_7648` | 0x44 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_768C` | 0x44 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_76D0` | 0x8C | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_775C` | 0x90 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_77EC` | 0x338 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_7B24` | 0x60 | F | 2: fn_14_7168, fn_14_A6A8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_7B84` | 0xC | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_7B90` | 0x44 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_7BD4` | 0xE0 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_7CB4` | 0xD8 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_7D8C` | 0x260 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_7FEC` | 0x5C | H | 0: - | no body |
| `fn_14_8048` | 0x48 | H | 0: - | no body |
| `fn_14_8090` | 0x298 | F | 0: - | body (PortCDigitalGuardian.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_8328` | 0x5C | H | 0: - | no body |
| `fn_14_8384` | 0x3C | H | 0: - | no body |
| `fn_14_83C0` | 0x184 | B | 1: fn_14_8090 | cited only (PortCDigitalGuardian.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_8544` | 0x3C | H | 0: - | no body |
| `fn_14_8580` | 0x1B8 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_8738` | 0x5C | H | 0: - | no body |
| `fn_14_8794` | 0x1E4 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_8978` | 0x5C | H | 0: - | no body |
| `fn_14_89D4` | 0x29C | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_8C70` | 0x5C | H | 0: - | no body |
| `fn_14_8CCC` | 0x178 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_8E44` | 0x1D0 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9014` | 0x5C | H | 0: - | no body |
| `fn_14_9070` | 0xF4 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9164` | 0xFC | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9260` | 0x25C | F | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_94BC` | 0xFC | F | 0: - | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_95B8` | 0x7C | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9634` | 0x4 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9638` | 0x34 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_966C` | 0xC | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9678` | 0x12C | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_97A4` | 0x4C | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_97F0` | 0x140 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9930` | 0x64 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9994` | 0x78 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9A0C` | 0x3C | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9A48` | 0x18 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9A60` | 0x3C | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9A9C` | 0xC | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9AA8` | 0x14 | F | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9ABC` | 0x1E8 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9CA4` | 0x118 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9DBC` | 0x40 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9DFC` | 0x8C | H | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9E88` | 0x15C | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_9FE4` | 0x20 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_A004` | 0x134 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_A138` | 0x210 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_A348` | 0x3C | H | 0: - | no body |
| `fn_14_A384` | 0x20 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_A3A4` | 0x30 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_A3D4` | 0x94 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_A468` | 0x154 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_A5BC` | 0xCC | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_A688` | 0x20 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_A6A8` | 0x4F0 | B | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_AB98` | 0x2F0 | B | 0: - | no body |
| `fn_14_AE88` | 0x1E4 | B | 2: fn_14_17F64, fn_14_AB98 | no body |
| `fn_14_B06C` | 0x94 | H | 1: fn_14_AE88 | no body |
| `fn_14_B100` | 0xF90 | B | 1: fn_14_1B8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_C090` | 0x54 | H | 0: - | no body |
| `fn_14_C0E4` | 0x6C | H | 0: - | no body |
| `fn_14_C150` | 0x54 | H | 0: - | no body |
| `fn_14_C1A4` | 0x6C | H | 0: - | no body |
| `fn_14_C210` | 0x58 | H | 0: - | no body |
| `fn_14_C268` | 0x8C | H | 0: - | no body |
| `fn_14_C2F4` | 0x8C | H | 0: - | no body |
| `fn_14_C380` | 0x3C | H | 0: - | no body |
| `fn_14_C3BC` | 0x3C | H | 0: - | no body |
| `fn_14_C3F8` | 0x3C | H | 0: - | no body |
| `fn_14_C434` | 0x3C | H | 0: - | no body |
| `fn_14_C470` | 0x3C | H | 0: - | no body |
| `fn_14_C4AC` | 0x78 | H | 0: - | no body |
| `fn_14_C524` | 0x58 | H | 0: - | no body |
| `fn_14_C57C` | 0x94 | B | 1: fn_14_B100 | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_C610` | 0x7C | B | 1: fn_14_C57C | no body |
| `fn_14_C68C` | 0x470 | B | 1: fn_14_B100 | cited only (PortCDigitalGuardian.cpp) |
| `fn_14_CAFC` | 0x2BC | B | 2: fn_14_1912C, fn_14_C68C | no body |
| `fn_14_CDB8` | 0x64 | B | 1: fn_14_C68C | no body |
| `fn_14_CE1C` | 0x78 | B | 1: fn_14_3DA8 | body (PortCDigitalGuardian.cpp) |
| `fn_14_CE94` | 0x100 | B | 1: fn_14_3A44 | body (PortCDigitalGuardian.cpp) |
| `fn_14_CF94` | 0x38C | B | 0: - | cited only (PortCDigitalGuardianKneeArmor.cpp) |
| `fn_14_D320` | 0x140 | B | 0: - | cited only (PortCDigitalGuardianKneeArmor.cpp) |
| `fn_14_D460` | 0x70 | H | 0: - | cited only (PortCDigitalGuardianKneeArmor.cpp) |
| `fn_14_D4D0` | 0x194 | B | 1: fn_14_4080 | cited only (PortCDigitalGuardianKneeArmor.cpp) |
| `fn_14_D664` | 0x110 | B | 1: fn_14_B100 | body (PortCDigitalGuardian.cpp) |
| `fn_14_D774` | 0x48 | B | 0: - | cited only (PortCDigitalGuardianKneeArmor.cpp) |
| `fn_14_D7BC` | 0x20 | B | 0: - | cited only (PortCDigitalGuardianKneeArmor.cpp) |
| `fn_14_D7DC` | 0xC0 | H | 3: fn_14_13318, fn_14_5C30, fn_14_61DC | cited only (PortCDigitalGuardian.cpp, PortCDigitalGuardianHead.cpp) |
| `fn_14_D89C` | 0x68 | B | 1: fn_14_D7DC | no body |
| `fn_14_D904` | 0x668 | H | 0: - | body (PortCDigitalGuardian.cpp) |
| `fn_14_DF6C` | 0x8 | B | 0: - | no body |
| `fn_14_DF74` | 0x8 | B | 0: - | no body |
| `fn_14_DF7C` | 0x344 | B | 0: - | cited only (PortCDigitalGuardian.cpp, PortCDigitalGuardianHead.cpp) |
| `fn_14_E2C0` | 0x920 | B | 1: fn_14_DF7C | no body |
| `fn_14_EBE0` | 0x190 | B | 3: fn_14_17F64, fn_14_1A5F4, fn_14_DF7C | no body |
| `fn_14_ED70` | 0x348 | B | 1: fn_14_DF7C | no body |
| `fn_14_F0B8` | 0x180 | B | 1: fn_14_E2C0 | no body |
| `fn_14_F238` | 0x3C | H | 1: fn_14_EBE0 | no body |
| `fn_14_F274` | 0x44 | B | 1: fn_14_ED70 | no body |
| `fn_14_F2B8` | 0xBC | B | 1: fn_14_1303C | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_F374` | 0x1C | B | 1: fn_14_1569C | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_F390` | 0x94 | B | 1: fn_14_14B98 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_F424` | 0x280 | B | 1: fn_14_15E18 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_F6A4` | 0x3AC | B | 1: fn_14_16A50 | no body |
| `fn_14_FA50` | 0x9C | B | 1: fn_14_17BBC | no body |
| `fn_14_FAEC` | 0x84 | B | 3: fn_14_16A50, fn_14_17D38, fn_14_FB70 | cited only (PortCDigitalGuardianHead.cpp) |
| `fn_14_FB70` | 0x2A4 | B | 1: fn_14_16A50 | no body |
| `fn_14_FE14` | 0x1E8 | B | 1: fn_14_FB70 | no body |
| `fn_14_FFFC` | 0x90 | B | 2: fn_14_18264, fn_14_FE14 | no body |
| `fn_14_1008C` | 0x94 | B | 1: fn_14_FE14 | no body |
| `fn_14_10120` | 0xC0 | B | 3: fn_14_15448, fn_14_15598, fn_14_16F74 | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_101E0` | 0x29C | B | 1: fn_14_16408 | no body |
| `fn_14_1047C` | 0x48 | B | 3: fn_14_101E0, fn_14_10A2C, fn_14_111E0 | no body |
| `fn_14_104C4` | 0xB4 | B | 2: fn_14_1047C, fn_14_11170 | no body |
| `fn_14_10578` | 0x40 | B | 2: fn_14_104C4, fn_14_110CC | no body |
| `fn_14_105B8` | 0x20 | B | 1: fn_14_10578 | no body |
| `fn_14_105D8` | 0x28 | B | 1: fn_14_105B8 | no body |
| `fn_14_10600` | 0x80 | B | 1: fn_14_105D8 | no body |
| `fn_14_10680` | 0x5C | H | 3: fn_14_1047C, fn_14_10744, fn_14_11170 | no body |
| `fn_14_106DC` | 0x20 | B | 1: fn_14_10680 | no body |
| `fn_14_106FC` | 0x48 | B | 1: fn_14_106DC | no body |
| `fn_14_10744` | 0x54 | H | 5: fn_14_101E0, fn_14_10A2C, fn_14_10CA0, fn_14_111E0, fn_14_1998C | no body |
| `fn_14_10798` | 0x294 | B | 1: fn_14_16408 | no body |
| `fn_14_10A2C` | 0x274 | B | 1: fn_14_17BBC | no body |
| `fn_14_10CA0` | 0x3A4 | B | 1: fn_14_17BBC | no body |
| `fn_14_11044` | 0x24 | B | 1: fn_14_10CA0 | no body |
| `fn_14_11068` | 0x64 | B | 1: fn_14_11044 | no body |
| `fn_14_110CC` | 0xA4 | B | 1: fn_14_10CA0 | no body |
| `fn_14_11170` | 0x70 | B | 1: fn_14_10CA0 | no body |
| `fn_14_111E0` | 0x2E8 | B | 1: fn_14_17BBC | no body |
| `fn_14_114C8` | 0x1D0 | B | 1: fn_14_14254 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_11698` | 0xF0 | B | 3: fn_14_118B0, fn_14_143D8, fn_14_15284 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_11788` | 0x128 | B | 3: fn_14_118B0, fn_14_143D8, fn_14_15284 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_118B0` | 0x134 | B | 1: fn_14_14254 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_119E4` | 0x1C4 | B | 1: fn_14_13830 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_11BA8` | 0x60 | H | 0: - | no body |
| `fn_14_11C08` | 0x15C | B | 2: fn_14_11D64, fn_14_16408 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_11D64` | 0x144 | B | 7: fn_14_15B94, fn_14_1633C, fn_14_167EC, fn_14_16A50, fn_14_16D64, fn_14 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_11EA8` | 0x88 | B | 1: fn_14_17D38 | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_11F30` | 0x4C | B | 1: fn_14_1303C | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_11F7C` | 0x70 | B | 1: fn_14_17D38 | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_11FEC` | 0x250 | B | 1: fn_14_1303C | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_1223C` | 0xD8 | B | 1: fn_14_17C98 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_12314` | 0x3B4 | B | 1: fn_14_17D38 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_126C8` | 0x150 | B | 1: fn_14_12818 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_12818` | 0x2EC | B | 1: fn_14_17C98 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_12B04` | 0x294 | B | 1: fn_14_1303C | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_12D98` | 0x248 | B | 1: fn_14_17D38 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_12FE0` | 0x5C | H | 0: - | no body |
| `fn_14_1303C` | 0x20C | B | 1: fn_14_17D38 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_13248` | 0x3C | B | 1: fn_14_1303C | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_13284` | 0x94 | B | 1: fn_14_13318 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_13318` | 0x490 | B | 1: fn_14_17D38 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_137A8` | 0x88 | B | 1: fn_14_18264 | body (PortCDigitalGuardianHelpers.cpp) |
| `fn_14_13830` | 0x194 | B | 1: fn_14_17D38 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_139C4` | 0xB0 | H | 1: fn_14_13830 | no body |
| `fn_14_13A74` | 0xC | B | 1: fn_14_139C4 | no body |
| `fn_14_13A80` | 0x60 | H | 0: - | no body |
| `fn_14_13AE0` | 0x28 | B | 1: fn_14_17C98 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_13B08` | 0x28 | B | 1: fn_14_7B24 | body (PortCDigitalGuardian.cpp) |
| `fn_14_13B30` | 0x8 | B | 6: fn_14_70E4, fn_14_7168, fn_14_7648, fn_14_768C, fn_14_76D0, fn_14_775C | body (PortCDigitalGuardian.cpp) |
| `fn_14_13B38` | 0xE4 | B | 13: fn_14_10A2C, fn_14_10CA0, fn_14_111E0, fn_14_11C08, fn_14_11D64, fn_14 | body (PortCDigitalGuardianHead.cpp, PortCDigitalGuardianHelpers.cpp) |
| `fn_14_13C1C` | 0x50 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_13C6C` | 0x24C | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_13EB8` | 0x54 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_13F0C` | 0x64 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_13F70` | 0xE8 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14058` | 0x1F0 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14248` | 0xC | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14254` | 0x184 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_143D8` | 0x98 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14470` | 0x134 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_145A4` | 0x60 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14604` | 0x60 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14664` | 0x1C4 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14828` | 0x1D0 | F | 0: - | cited only (PortCDigitalGuardianHead.cpp) |
| `fn_14_149F8` | 0x168 | F | 0: - | cited only (PortCDigitalGuardianHead.cpp) |
| `fn_14_14B60` | 0x38 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14B98` | 0x8C | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14C24` | 0xD0 | F | 0: - | cited only (PortCDigitalGuardianHead.cpp) |
| `fn_14_14CF4` | 0xAC | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14DA0` | 0xD8 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14E78` | 0xFC | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_14F74` | 0xE0 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_15054` | 0x1F8 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_1524C` | 0x38 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_15284` | 0x14C | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_153D0` | 0x78 | F | 0: - | cited only (PortCDigitalGuardianHead.cpp) |
| `fn_14_15448` | 0x150 | F | 0: - | cited only (PortCDigitalGuardianHead.cpp) |
| `fn_14_15598` | 0x74 | F | 1: fn_14_13B08 | cited only (PortCDigitalGuardianHead.cpp) |
| `fn_14_1560C` | 0x10 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_1561C` | 0x80 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_1569C` | 0x25C | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_158F8` | 0x29C | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_15B94` | 0x154 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_15CE8` | 0x130 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_15E18` | 0x124 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_15F3C` | 0x224 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_16160` | 0x1DC | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_1633C` | 0xCC | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_16408` | 0x3E4 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_167EC` | 0x208 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_169F4` | 0x5C | F | 0: - | no body |
| `fn_14_16A50` | 0x314 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_16D64` | 0x210 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_16F74` | 0x128 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_1709C` | 0x98 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17134` | 0x4C | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17180` | 0x4 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17184` | 0x14 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17198` | 0x20 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_171B8` | 0x20 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_171D8` | 0x14 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_171EC` | 0xC | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_171F8` | 0x4C | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17244` | 0x48 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_1728C` | 0xC | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17298` | 0xA0 | F | 4: fn_14_14470, fn_14_14B98, fn_14_15598, fn_14_15E18 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17338` | 0x14 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_1734C` | 0x10 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_1735C` | 0x14 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17370` | 0x14 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17384` | 0x14 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17398` | 0x14 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_173AC` | 0xC | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_173B8` | 0x14 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_173CC` | 0x3C | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17408` | 0x4C | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17454` | 0x14 | F | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17468` | 0x8C | B | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_174F4` | 0x144 | B | 0: - | no body |
| `fn_14_17638` | 0x190 | B | 0: - | no body |
| `fn_14_177C8` | 0x78 | B | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17840` | 0x50 | B | 0: - | no body |
| `fn_14_17890` | 0xE4 | B | 0: - | no body |
| `fn_14_17974` | 0x6C | B | 1: fn_14_17890 | no body |
| `fn_14_179E0` | 0x20 | B | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17A00` | 0x44 | B | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17A44` | 0x20 | B | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17A64` | 0xC4 | B | 0: - | no body |
| `fn_14_17B28` | 0x94 | B | 0: - | no body |
| `fn_14_17BBC` | 0xDC | B | 0: - | no body |
| `fn_14_17C98` | 0xA0 | B | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17D38` | 0x22C | B | 0: - | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_17F64` | 0x300 | H | 0: - | no body |
| `fn_14_18264` | 0xE7C | H | 1: fn_14_DF7C | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_190E0` | 0x4C | B | 1: fn_14_18264 | cited only (PortCDigitalGuardianHead.cpp) |
| `fn_14_1912C` | 0x5C4 | B | 2: fn_14_18264, fn_14_1A698 | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_196F0` | 0x94 | H | 1: fn_14_1912C | no body |
| `fn_14_19784` | 0xB4 | H | 0: - | no body |
| `fn_14_19838` | 0xD0 | H | 1: fn_14_10CA0 | no body |
| `fn_14_19908` | 0x84 | H | 1: fn_14_1AAC4 | no body |
| `fn_14_1998C` | 0x7C | H | 1: fn_14_19908 | no body |
| `fn_14_19A08` | 0x168 | H | 1: fn_14_1A1F8 | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_19B70` | 0x170 | H | 1: fn_14_1A1F8 | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_19CE0` | 0x118 | H | 1: fn_14_1A1F8 | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_19DF8` | 0x120 | H | 2: fn_14_14CF4, fn_14_14DA0 | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_19F18` | 0xB8 | H | 1: fn_14_1223C | body (PortCDigitalGuardianHead.cpp) |
| `fn_14_19FD0` | 0x15C | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1A12C` | 0x3C | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1A168` | 0x90 | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1A1F8` | 0x238 | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1A430` | 0x1C4 | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1A5F4` | 0xA4 | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1A698` | 0x3AC | H | 1: fn_14_12314 | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1AA44` | 0x40 | H | 0: - | no body |
| `fn_14_1AA84` | 0x40 | H | 0: - | no body |
| `fn_14_1AAC4` | 0x60 | H | 0: - | no body |
| `fn_14_1AB24` | 0xC | H | 0: - | no body |
| `fn_14_1AB30` | 0x20 | H | 0: - | no body |
| `fn_14_1AB50` | 0x48 | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1AB98` | 0x40 | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1ABD8` | 0x8 | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `fn_14_1ABE0` | 0x34 | H | 1: fn_14_18264 | no body |
| `fn_14_1AC14` | 0xA4 | H | 1: fn_14_139C4 | no body |
| `fn_14_1ACB8` | 0x74 | H | 1: fn_14_19908 | no body |
| `fn_14_1AD2C` | 0x9CC | H | 0: - | cited only (PortCDigitalGuardianEchoTarget.cpp) |
| `_unresolved` | 0xC4 | T | 0: - | no body |
| `_epilog` | 0x24 | T | 0: - | no body |
| `_prolog` | 0x24 | T | 0: - | no body |
| `ModuleDestructors` | 0x4C | T | 0: - | no body |
| `ModuleConstructors` | 0x4C | T | 0: - | no body |

### 3.4 The gcov profile (the coverage build)

The compiler coverage build (`-B build-port-cov -DMP_PORT_COVERAGE=ON`) gives the branch
coverage of every port body at once. The run:

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/digitalguardian.py /tmp/opencode/dg_gcov1 \
      1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_area53.con --fast --frames 3000
    BUILD=build-port-cov COV_GDB_SCRIPT=tools/cov/force_cov.py tools/cov/force.sh \
      tools/cov/specs/digitalguardian_force.py /tmp/opencode/dg_gcov2 1baa96c2,09_cliff_temple_dark \
      --con tools/cov/specs/digitalguardian_area53.con --fast --frames 3000
    tools/cov/gcov_report.py build-port-cov /tmp/opencode/dg_gcov1/gcov /tmp/opencode/dg_gcov2/gcov \
      --module DigitalGuardian

| id | the gcov branches | the untaken edges | what the edges are |
|---|---|---|---|
| `fn_14_6CB0` | **8/8 ALL-ARMS** | - | - |
| `fn_14_11F7C` | **2/2 ALL-ARMS** | - | - |
| `fn_14_6C74` | 2/4 | 52, 55 | the two forced arms (the vortex mode, the flag byte's bit 6) |
| `fn_14_3F90` | 4/6 | 72, 76 | the two forced arms (an invalid knee id, a knee that is gone) |
| `fn_14_2358` | 5/10 | 124, 132, 136 | the three forced arms (no emitter, a positive timer, a toe target that does not flinch) |
| `fn_14_6C24` | 1/2 | 206 | the forced arm (the body state is 0) |
| `fn_14_11EA8` | 3/4 | 353 | the forced arm (a bomb slot with a sound) |
| `fn_14_38BC` | 1/10 | 97, 101, 104, 106, 108 | the profile of this body is not usable (the natural arms are missing too) |
| `fn_14_6E04` | 7/12 | 290 | the arm that needs the head's projectile attack |
| `fn_14_5644` | 1/24 | 157, 160, 164, 167, 170, 173, 174, 176, 178, 181, 187, 193 | the arms past the first early-out |
| `fn_14_37FC` | 0/6 | 35, 36, 37 | the body has no caller yet |
| `fn_14_10120` | 0/10 | 309, 312, 315 | the body has no caller yet |
| `fn_14_F374` | 0/2 | 339 | the body has no caller yet |
| `fn_14_137A8` | 2/3 | 447 | the forced arm (the armour id is 0xFFFE) |
| `fn_14_83C0` | 0/0 | - | the body has no caller yet |

**The gcov profile of the forced arms is missing, and that is a tooling gap, not a code fact.**
In the coverage build the gdb breakpoints on the *body's* helper entries do not fire (the force
run's `cov.txt` shows 0 hits for `x3F90_`, `x6C74_`, `x38BC_`, `x2358_`, `x5644_`, `x6C24_`,
`x6CB0_`, `x6E04_`, `x11EA8_` and `x11F7C_`, and its `forced.log` has only the head's
`fn_14_137A8` line), so the forced members were never written in that build and the profile is
the plain run's. The forced arms are therefore proven by the gdb force run on `build-port-link`
(the keys and the call-site counts in section 3.2), and the gcov rows above show exactly which
edges the plain run leaves out. The two bodies the gcov confirms outright (`fn_14_6CB0` and
`fn_14_11F7C`) are ALL-ARMS.

### 3.5 The coverage build's gdb breakpoints (still open)

Two things went wrong in the coverage build, one of them mine:

1. **My spec file did not parse.** The first force run on `build-port-cov` ended with
   `Python Exception <class 'IndentationError'>: unexpected indent (digitalguardian_force.py,
   line 116)`: a stray `]` had ended up on the same line as a comment, so gdb's script aborted
   before any breakpoint was installed and the run looked like "no coverage at all". The spec
   parses now (`python3 -c "compile(open(...).read(),'x','exec')"` is the check).
2. **The coverage harness's counters stay 0 in a forcing run.** With the spec fixed, the force
   run on the coverage build shows 0 hits for every one of the body's helper entries, while the
   plain run on the *same binary* shows 400 (`tools/cov/run.sh` with the same spec's SPEC list).
   The breakpoints are set at the right addresses: the force run's log has 30 `Breakpoint N at
   0x...` lines, each with the right source line, and they match the `nm` rows of the coverage
   build (for example `x2358_` at 0xbb3c30). The forcing side (`tools/cov/force.py`) and the
   coverage side (`tools/cov/gdb_cov.py`) are installed by `tools/cov/cov_force.py` in one gdb
   session; the forcing's breakpoints were visible ones (`gdb.Breakpoint(spec,
   internal=False)`), which stop the game on every hit - the game then crawls, the console
   times out and the run is killed by its timeout. They are internal now (the commit "tools/cov:
   the forcing breakpoints must be internal"), which removes the visible stop, but the counters
   still stay 0: the rest of the cause is still open.
   The forcing runs that work are the ones on `build-port-link` (the eight runs of section 3.2
   and the nine runs of section 3.6), and the orchestrator's ruling is that a gdb force run is
   valid evidence.

### 3.6 `fn_14_5644`: one force run per arm

A member a force writes stays written, so the arms of `fn_14_5644` need one run each. The nine
runs (`tools/cov/force.sh` with the specs `/tmp/opencode/dg5644_<arm>.py`, area 53, 3000 frames;
the same SPEC list, one FORCE entry each) and the call sites each run reached:

| run | the forced members | the calls after the lookup | the arm |
|---|---|---|---|
| (no force) | - | none | (a) the sender is not a collider |
| `b_knee` | the sender is a knee collider | `x43AC_` | (b) a collider but not a toe target |
| `c_skipped` | the sender is a toe target, its word is 3 | `x43AC_`, `x4350_` | (c) the toe target is skipped |
| `d_timer` | the sender is a toe target, its timer is 5 | `x43AC_`, `x4350_` | (d) the hit timer is running |
| `e_notmorphed` | the sender is a toe target | `x43AC_`, `x4350_` | (e) the player is not morphed |
| `f_noshield` | the player is morphed | `+ CMorphBall::IsBoostShieldActive` | (f) morphed without a boost shield |
| `g_boost` | the player is morphed with its boost shield | `+ CTweakBall::GetBoostBallDamage`, `CDamageInfo::GetVulnerableDamage`, `CActor::PlayCustomSound`, `x1A68_` | (h) the toe target survives: the sound and the flinch |
| `h_dies` | the toe target has no health | `+ x1A68_`, `x143C_` | (g) the toe target dies: the explosion and the IS08 message |
| `i_noflinch` | the toe target's word is 2 | `+ PlayCustomSound` (no `x1A68_`) | (i) the toe target survives without the flinch |

Every arm ran, so `fn_14_5644` is **marked**. The retail's third early-out (the toe index is
-1) is dead in the port as well: `fn_14_43AC` (the sender is one of the four toe target
colliders) is false for every id `fn_14_4350` does not know.

## 7. The unmarked bodies that are one run away (for the FSM lane)

`tools/cov/gcov_report.py build-port-cov <the prefix dirs> --module DigitalGuardian` prints the
branch coverage of every body of the module; the unmarked bodies that are already ALL-ARMS or
one edge short are the ones the lane can mark with one more run:

| retail id | the body | branches | missing edges | the note |
|---|---|---|---|---|
| `fn_14_A5BC` | `CDigitalGuardian::Think(float, CStateManager&)` | 4/4 | ALL-ARMS |
| `fn_14_95B8` | `CDigitalGuardian::Idle(CStateManager&, EStateMsg, fl` | 3/3 | ALL-ARMS |
| `fn_14_12314` | `CDigitalGuardianHead::x12314_(CStateManager&)` | 13/14 | one edge short: untaken @ 737 |
| `fn_14_1B8` | `PortLoadDigitalGuardian(CStateManager&, CInputStream` | 10/11 | one edge short: untaken @ 2539 |
| `fn_14_DF7C` | `PortLoadDigitalGuardianHead(CStateManager&, CInputSt` | 9/10 | one edge short: untaken @ 1487 |
| `fn_14_D664` | `CDigitalGuardian::xD664_(unsigned int)` | 4/5 | one edge short: untaken @ 1136 |
| `fn_14_1994` | `CDigitalGuardian::x1994_(CStateManager const&, CTran` | 3/4 | one edge short: untaken @ 1019 |
| `fn_14_173CC` | `CDigitalGuardianHead::ShouldProjectileAttack(CStateM` | 1/2 | one edge short: untaken @ 890 |
| `fn_14_177C8` | `CDigitalGuardianHead::GetScannableObjectInfo() const` | 1/2 | one edge short: untaken @ 851 |
| `fn_14_2578` | `CDigitalGuardian::x2578_(CTransform4f&, CTransform4f` | 1/2 | one edge short: untaken @ 1119 |
| `fn_14_3974` | `CDigitalGuardian::x3974_(CStateManager&, bool)` | 1/2 | one edge short: untaken @ 628 |
| `fn_14_9930` | `CDigitalGuardian::ShouldVortexAttack(CStateManager&,` | 1/2 | one edge short: untaken @ 1480 |
| `fn_14_9994` | `CDigitalGuardian::ShouldProtectKnees(CStateManager&,` | 1/2 | one edge short: untaken @ 1468 |

The module has 13 such bodies (not 20): the other unmarked bodies are further away (the head's
states and codes, the sub-actor constructors, the effect helpers).
