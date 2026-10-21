# DarkSamus REL (module 10): the stateless helpers (this lane's half)

This note is the ex-Sandworm lane's half of the DarkSamus module: the **class-B rows of
`DarkSamus_functions.tsv` with an empty `state` column and an offset below 0xA000** - the boss's
helpers, not its FSM handlers. 96 rows (95 unmarked, plus `fn_10_9644`, which the lane marked).
The functions at and above 0xA000, and every row with a `state`, belong to the other halves.

The lane's own note is `docs/module-analysis/DarkSamus.md`; this one does not edit it, and it does
not edit `DarkSamus_functions.tsv` (the orchestrator merges the tsv).

## 1. How each row was classified

Three scans, all over the module's own build in the archive tree
(`<archive>/build/G2ME01/DarkSamus/asm`):

1. **Direct calls.** `grep 'bl fn_10_XXXX'` over the module's `.text`: the retail call sites.
2. **Table references.** The module's `.data` (the vtables and the FSM registration tables). Only
   **`fn_10_0`** of these 96 is in a table (the vtable's slot 15); the other 95 are reached only by
   a direct call, so they are helpers in the strict sense.
3. **The port's state.** What the tree already has for each row: a body (`PortCDarkSamus*.cpp`), a
   stand-in (`FsmStandIn`), a declaration only, or nothing.

## 2. The rows

| id | size | kind | retail callers | the port | decision |
|---|---|---|---|---|---|
| `fn_10_0` | 0x2C | virtual | 0 | ported body | **marked** - the vtable slot 15; the body is the base's own accessor |
| `fn_10_2C` | 0x29C | method | 1 | ported body | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_2C8` | 0x5C | method | 15 | ported body | the module's copy of the DOL's CRayCastResult copy constructor - a host copy |
| `fn_10_324` | 0x6E4 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_A08` | 0x58 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_A60` | 0x5C | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_B04` | 0x24 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_B28` | 0x50 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_B78` | 0xE4 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_C98` | 0x130 | method | 2 | cited | needs the module's own CScattershotProjectile class (57 functions): the port keeps the stand-in |
| `fn_10_DC8` | 0x48 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_E10` | 0x4C | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_1B24` | 0x40 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_1B64` | 0xA4 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_1C08` | 0x184 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_1D8C` | 0x2B4 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_2040` | 0x58 | method | 7 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_2098` | 0x54 | method | 3 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_213C` | 0x118 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_2254` | 0x544 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_2798` | 0x180 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_2AD0` | 0x374 | method | 1 | ported body | needs the module's own CScattershotProjectile class: the port creates a CEnergyProjectile instead |
| `fn_10_2E44` | 0x20 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_2EFC` | 0x8 | method | **ported** |
| `fn_10_2F04` | 0xC | method | **ported** |
| `fn_10_2F10` | 0xF8 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_31E4` | 0x304 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_34E8` | 0x38 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_3520` | 0x20 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_3540` | 0x28 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_3568` | 0xC0 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_3628` | 0x84 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_36AC` | 0x38 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_36E4` | 0x60 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_3744` | 0x5C | method | 5 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_37A0` | 0x130 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_3BC4` | 0x3AC | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_3F70` | 0x68 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_3FD8` | 0x390 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4368` | 0x40 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_43A8` | 0xC0 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4468` | 0x68 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_452C` | 0xCC | method | blocked - it resets the 0xEF0 sub-object's members (0xF10..0xF5C), which are not modelled yet |
| `fn_10_45F8` | 0x270 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4868` | 0x60 | method | **ported** |
| `fn_10_48C8` | 0x44 | method | **ported** |
| `fn_10_490C` | 0x19C | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4AA8` | 0x14 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4C40` | 0x15C | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4D9C` | 0x54 | method | 4 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4DF0` | 0x5C | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4E4C` | 0x20 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4E6C` | 0x48 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4EB4` | 0x48 | method | 4 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4EFC` | 0xB4 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4FB0` | 0x40 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4FF0` | 0x20 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_5010` | 0x28 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_5038` | 0x80 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_50B8` | 0x788 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_5840` | 0x80 | method | 1 | cited | the anim event's melee/missile arm: the attacks have no ProjectileInfo in the port yet |
| `fn_10_58C0` | 0xB4 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_5974` | 0x7A4 | method | blocked - the 0xEF0 sub-object's members (0xEF8, 0xF10..0xF58) are not modelled yet |
| `fn_10_6118` | 0x19C | method | 1 | ported body | the anim event's melee/missile arm: the attacks have no ProjectileInfo in the port yet |
| `fn_10_6374` | 0xEC | method | **ported** (the lane's fn_10_1DD14 / 1D860 / 11334 stand in) |
| `fn_10_65CC` | 0x1EC | method | to port - the base class's members only; the host's names are known, the body is 0x1EC bytes |
| `fn_10_69C0` | 0x80 | method | **ported** |
| `fn_10_6DAC` | 0x8C | method | blocked - the values it copies come from the module's global bss_128, which the host has no member for |
| `fn_10_6F90` | 0xC4 | method | **ported** (the guest's 0x6F4/0x1048 map to the host's CPathFindNavigation) |
| `fn_10_7304` | 0x168 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_746C` | 0xCC | method | blocked - it calls fn_10_7304 (this half, not ported yet) |
| `fn_10_7538` | 0xD8 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_7610` | 0x170 | method | blocked - it calls fn_10_8D30 and fn_10_9E00 (this half, not ported yet) |
| `fn_10_7780` | 0x38 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_77B8` | 0xB4 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_786C` | 0x1A8 | method | blocked - it calls fn_10_7F98 (this half, not ported yet) |
| `fn_10_7D00` | 0x298 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_7F98` | 0x17C | method | blocked - it calls the lane's fn_10_1B478 / 1AEC0 / 18F90 and this half's fn_10_7538 / 7610 / 7D00 |
| `fn_10_812C` | 0x4C | method | **ported** |
| `fn_10_8178` | 0x5C | method | 1 | ported body | the anim event's melee/missile arm: the attacks have no ProjectileInfo in the port yet |
| `fn_10_8698` | 0xCC | method | **ported** (the guest's command words become the host's CBCTauntCmd + mAnimationState) |
| `fn_10_8764` | 0x10C | method | the port already has the host form (the lane's `RunMeleeAttackCmd`) |
| `fn_10_8870` | 0xEC | method | blocked - its command words (0x1B8/0x1C0/0x1C4) map to a host CBC*Cmd that is not identified yet |
| `fn_10_895C` | 0x3D4 | method | 1 | declared only | the boost ball sub-object's constructor; the host keeps two of its members |
| `fn_10_8D30` | 0x84 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_8DB4` | 0x19C | method | 1 | cited | the sweep beam sub-object's constructor; the host keeps two of its members |
| `fn_10_8F50` | 0x214 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_9164` | 0x1B0 | method | 1 | cited | the anim event's melee/missile arm: the attacks have no ProjectileInfo in the port yet |
| `fn_10_9624` | 0x20 | method | 6 | ported body | ported body in PortCDarkSamusHelpers.cpp; no caller in the port yet |
| `fn_10_9644` | 0xE0 | method | ported and marked by the lane |
| `fn_10_9920` | 0x4C | method | ported and marked here |
| `fn_10_996C` | 0x4C | method | 4 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_99B8` | 0x10C | method | 3 | ported body | stops the boost ball's particle effects: the host has no such effects |
| `fn_10_9AC4` | 0x60 | method | ported and marked here |
| `fn_10_9B24` | 0x2DC | method | 1 | ported body | builds the boost ball's particle effects: the host has no such effects |
| `fn_10_9E00` | 0x26C | method | blocked - it calls fn_10_1D860 (the lane's half) and fn_10_6F90 (this half) |

## 3. The marked ones, with their arms

The run: **Sanctuary 10_cliff_rooftop layer 2** (`MP2_WORLD="1baa96c2,10_cliff_rooftop"`,
`MP2_INPUT="LAYER2@400"`), boss **u04f7**, `MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8`, the lane's
driver `tools/drivers/darksamus_boost_rooftop.con` (the boost ball) and
`tools/drivers/darksamus_boost_dive.con` (the dive out of it). Coverage was read with gdb
breakpoints on the port's own symbols - no probe was committed. The forced arm is labelled
**forced**.

| id | the arms | how they ran |
|---|---|---|
| `fn_10_0` (vtable slot 15, `CDarkSamus::GetHealthInfo`) | one: the retail calls the vtable's word 14 (0x38) and returns its result (0x0000000C..0x0000001C); the port calls the base's accessor, which is that slot | every health query in the fight |
| `fn_10_9920` (`GetBoostBallActor`) | two: the ball's uid (0xE38) is kInvalidUniqueId -> null; else the actor it names | the null arm naturally (the boost ball paths run with no ball); the actor arm **forced** (gdb `set var mBoostBallUid.value = 0x4f7` at the second call), 644 calls |
| `fn_10_9AC4` (`DeleteBoostBall`) | two: the uid is invalid -> nothing; else the actor is deleted and the uid cleared | the null arm naturally (the dive's exit calls it with no ball); the delete arm **forced** (gdb `set var mBoostBallUid.value = 0x4f7` at the dive's entry) |

## 4. What is left, and why

- **79 rows have no body at all.** Their callers are the module's FSM states or helpers the port
  does not have yet, so a body could not be run (and so could not be marked). They are listed above
  with their retail caller count; the lane can wire them one at a time.
- **Six rows need the module's own classes**, which the host does not have:
  `fn_10_C98` and `fn_10_2AD0` (the module's `CScattershotProjectile`, 57 functions - the port
  fires a `CEnergyProjectile` instead), `fn_10_895C` and `fn_10_8DB4` (the boost ball and the sweep
  beam sub-objects' constructors - the host keeps two members of each), `fn_10_99B8` and
  `fn_10_9B24` (the boost ball's particle effects, which the host does not have).
- **Four rows are the anim event's melee/missile arms** (`fn_10_5840`, `fn_10_6118`, `fn_10_8178`,
  `fn_10_9164`). The port's `DoUserAnimEvent` skips them because the attacks have no
  `ProjectileInfo` in the host - wiring them would make the base's `LaunchProjectile` read a null
  one, so they wait for the projectile infos.
- **Two rows are the lane's half in practice**: `fn_10_8698` (the taunt command) and
  `fn_10_8764` (the melee command over a state's messages) run inline in the lane's state bodies;
  they can be marked when the lane moves them into a body of their own.
- **`fn_10_2C8`** is the module's copy of the DOL's `CRayCastResult` copy constructor - a host copy
  in the Sandworm note's rule set (H), not a port body.

## 5. The port's new file

`src/MetroidPrime/Enemies/PortCDarkSamusHelpers.cpp` (added to `files.cmake` through
`tools/modules_port.txt` and `python3 tools/gen_files_cmake.py`) holds the bodies this lane
ports; the header additions are in one `#ifdef TARGET_PC` block at the end of the class in
`include/MetroidPrime/Enemies/CDarkSamus.hpp`.

## 6. The helpers the FSM lane's next states call (the port order)

The FSM lane's next states are 62B4 (Arrived), 9724 (BoostBallFire), A3B0 (BoostBallRoll), 143A8
(Dead), 9314 (DoneRolling), 7A14 (EnrageHitReact), 6E38 (EnragedTravel) and then the phazon
states. These are the helpers of this half that those states call - the bodies to port first, in
the lane's order, so a state can be written against them:

| helper | size | called from | the port |
|---|---|---|---|
| `fn_10_8764` | 0x10C | 9724 @0x98E0, 7A14 @0x7B28, 67B8 @0x6924 | the port already has the host form (the lane's `RunMeleeAttackCmd`) |
| `fn_10_9E00` | 0x26C | A3B0 @0xA4AC | blocked - it calls fn_10_1D860 (the lane's half) and fn_10_6F90 (this half) |
| `fn_10_2F04` | 0xC | A3B0 @0xA4FC | **ported** |
| `fn_10_2EFC` | 0x8 | A3B0 @0xA548 | **ported** |
| `fn_10_7F98` | 0x17C | 7A14 @0x7AC8, 67B8 @0x68B4, 7BC0 @0x7CCC | blocked - it calls the lane's fn_10_1B478 / 1AEC0 / 18F90 and this half's fn_10_7538 / 7610 / 7D00 |
| `fn_10_6F90` | 0xC4 | 6E38 @0x6EBC, 6E38 @0x6F10 | **ported** (the guest's 0x6F4/0x1048 map to the host's CPathFindNavigation) |
| `fn_10_6DAC` | 0x8C | 6E38 @0x6F4C | blocked - the values it copies come from the module's global bss_128, which the host has no member for |
| `fn_10_65CC` | 0x1EC | 6460 @0x64A4, 652C @0x6570, 7054 @0x7190 | to port - the base class's members only; the host's names are known, the body is 0x1EC bytes |
| `fn_10_6374` | 0xEC | 6460 @0x64F0, 652C @0x65A4 | **ported** (the lane's fn_10_1DD14 / 1D860 / 11334 stand in) |
| `fn_10_69C0` | 0x80 | 67B8 @0x68A0 | **ported** |
| `fn_10_746C` | 0xCC | 6BA0 @0x6C04 | blocked - it calls fn_10_7304 (this half, not ported yet) |
| `fn_10_7610` | 0x170 | 6BA0 @0x6C10 | blocked - it calls fn_10_8D30 and fn_10_9E00 (this half, not ported yet) |
| `fn_10_4868` | 0x60 | 6BA0 @0x6D30 | **ported** |
| `fn_10_8698` | 0xCC | 6BA0 @0x6D6C, 7054 @0x72C4, 4AC8 @0x4B54 | **ported** (the guest's command words become the host's CBCTauntCmd + mAnimationState) |
| `fn_10_786C` | 0x1A8 | 7BC0 @0x7C58 | blocked - it calls fn_10_7F98 (this half, not ported yet) |
| `fn_10_452C` | 0xCC | 81D4 @0x8298 | blocked - it resets the 0xEF0 sub-object's members (0xF10..0xF5C), which are not modelled yet |
| `fn_10_5974` | 0x7A4 | 81D4 @0x8340 | blocked - the 0xEF0 sub-object's members (0xEF8, 0xF10..0xF58) are not modelled yet |
| `fn_10_48C8` | 0x44 | 81D4 @0x834C | **ported** |
| `fn_10_812C` | 0x4C | 81D4 @0x858C | **ported** |
| `fn_10_8870` | 0xEC | 81D4 @0x85DC | blocked - its command words (0x1B8/0x1C0/0x1C4) map to a host CBC*Cmd that is not identified yet |
| `fn_10_9644` | 0xE0 | 9724 @0x9788 | ported and marked by the lane |
| `fn_10_9920` | 0x4C | A3B0 @0xA4EC | ported and marked here |
| `fn_10_9AC4` | 0x60 | 9724 @0x984C | ported and marked here |

### 6.1 The bodies ported in this slice

All of them are in `src/MetroidPrime/Enemies/PortCDarkSamusHelpers.cpp`; the declarations are in the
`#ifdef TARGET_PC` block at the end of the class in `CDarkSamus.hpp` (append-only, so the FSM
lane's edits merge cleanly). They are **unmarked**: the port has no caller for them yet - the FSM
lane's states are what will call them.

| id | the body |
|---|---|
| `fn_10_2EFC` | the float at guest 0x314 (0x00002EFC) |
| `fn_10_2F04` | bit 0x80 of the byte at guest 0x318 (the extrwi at 0x00002F08 takes the bit 25) |
| `fn_10_2E44` | the `CModelData` default constructor (0x00002E50) |
| `fn_10_4868` | the actor whose uid is at guest 0xF78 is deleted and the uid cleared (0x000048A4, 0x000048B0); a kInvalidUniqueId uid is left alone (0x00004890) |
| `fn_10_48C8` | the same, but only when the timestamp at guest 0xF7C is older than the elapsed time (0x000048F0) |
| `fn_10_812C` | the sfx handle at guest 0xF60 is removed and cleared (0x00008158, 0x00008160); a null handle is left alone (0x00008148) |
| `fn_10_9624` | the boost ball's uid (guest 0xE38) is not kInvalidUniqueId (the subf/or/srwi idiom at 0x00009630..0x00009640) |
| `fn_10_69C0` | the uids at guest 0x934 are deleted from the state manager (0x00006A0C) and the slot cleared (0x00006A14); a kInvalidUniqueId slot is left alone (0x000069F4). The loop runs over the count at guest 0x930 (0x00006A20) |
| `fn_10_6374` | the path find along the waypoints: with the message 0 the destination is the module's vector at guest 0xEE4 (0x00006418), otherwise the path find runs with that message (0x000063BC); then the separation (the lane's fn_10_1DD14, 0x000063C8), the path check (0x000063E8) and, when the path is not shagged and the actor is on the last waypoint (0x000063F8..0x00006408), the destination is set again (0x0000641C) |
| `fn_10_8698` | the taunt command over a state's messages: Activate stores 1 (kAS_Ready) to the animation state 0x6B4 and delivers the command (0x000086E0, 0x000086F0), Update re-delivers it when the DOL's ShouldRetry answers true (0x00008710, the host's `CAnimationState::CanIssueCommand`), Deactivate stores 0 (kAS_NotReady) (0x00008750) |
| `fn_10_6F90` | the path find to the destination: the path find itself is `fn_10_1C778` (the host's `SetPathFindDestination`, 0x00006FC0); when the path is shagged (0x00006FE0) the navigation's destination is set again (0x00006FF8 - the guest's 0x1048 is the navigation's own destination, which the host keeps in `CPathFindNavigation`) and the path find runs again (0x00007010) |

The five members they need (guest 0x314, 0x318, 0xF60, 0xF78, 0xF7C) are not in the host class, so
they were added to the same `TARGET_PC` block (`mGuest314`, `mGuest318`, `mGuestF60`, `mGuestF78`,
`mGuestF7C`).

### 6.2 The host modelling that was applied

- **The retail's body-controller command words** become the host's `CBodyStateCmdMgr` API:
  `fn_10_8698`'s words (the command type and the taunt type, stored to 0x2B0/0x2B4) are the host's
  `CBCTauntCmd(pas::ETauntType)` handed to `DeliverCmd`, and the animation state 0x6B4 is
  `CPatterned::mAnimationState` (`SetState(kAS_Ready)` / `kAS_NotReady`); the re-delivery test is
  the host's `CAnimationState::CanIssueCommand`. `fn_10_8764` needs the same: the host's
  `CBCMeleeAttackCmd` already carries the severity and the target, and the lane's
  `RunMeleeAttackCmd` is its host form.
- **The base class's members** (the guest's 0x20..0x70 and the like) are the host's named members
  - the upstream headers carry the guest offsets in their `// xNN` comments (`mTransform` x24,
  `mPosition` x54, `mModelData` x60, `mMaterial` x68 in `CActor.hpp`), so the retail's offsets map
  one to one onto the host's members.
- **`fn_10_6F90`'s navigation** (the guest's 0x6F4 and 0x1048) is the host's
  `CPatterned::mPathFindNavigation` (`SetDestination`, `PathFind`, `GetDestinationPosition`), and
  `fn_10_1C778` is the port's `SetPathFindDestination` (it exists on port, in the lane's file).
- **What is still blocked** is listed per helper in the table above: the offsets with no host
  member (the module's own loader data and the stage info) and the calls into the FSM lane's half
  (`fn_10_1C778` exists; `fn_10_1DD14` does not yet).
### 6.3 The members the helpers need (the host modelling)

The orchestrator's call: the loader data and the stage info are modelled as named members in
`CDarkSamus.hpp`'s `TARGET_PC` block (append-only). The names are the guest offsets, because
neither the loader nor a DOL accessor names them:

| member | guest | type | initialiser |
|---|---|---|---|
| `mGuest930` | 0x930 | int | 0 - the number of uids in 0x934; the retail's ctor writes 0 here |
| `mGuest934` | 0x934 | ushort[18] | 0 - the uids `fn_10_69C0` deletes (the guest's array runs to 0x958) |
| `mGuestFAC` | 0xFAC | float | 0 - the scale `fn_10_746C` compares with |
| `mGuestFC8` | 0xFC8 | float | 0 (`fn_10_7610`) |
| `mGuestFCC` | 0xFCC | float | 0 (`fn_10_7F98`) |
| `mGuest1020` / `mGuest1024` / `mGuest1028` | 0x1020 / 0x1024 / 0x1028 | float | 0 (`fn_10_786C`, `fn_10_7610`) |
| `mGuestD14` | 0xD14 | float | 0 (`fn_10_9E00`) |
| `mGuestD1C` / `mGuestD20` | 0xD1C / 0xD20 | float | 0 - copies of the module's `bss_128[1]` and `[2]` (`fn_10_6DAC`) |
| `mGuestE2C` / `mGuestE30` / `mGuestE34` / `mGuestE38` | 0xE2C..0xE38 | float | 0 (`fn_10_9E00`) |
| `mGuestE6A` | 0xE6A | uchar | 0 - a flag byte (`fn_10_9E00`) |
| `mGuestEE4` | 0xEE4 | CVector3f | the zero vector - the path find's fallback destination (`fn_10_6374`) |

**No loader read has to go into the ctor.** The retail's ctor writes only 0x930 = 0 among these
(the rest are runtime state), and the in-class initialisers already cover it. The loader's
properties stay in `CDarkSamusData`, which the port reads in the ctor as it does now.

**Still not modelled** (the next slice's work): the 0xEF0 sub-object's members - 0xEF8 and
0xF10..0xF58, which `fn_10_452C` and `fn_10_5974` read and write (the loader's
`SLdrDSStageInfo` block). They need the loader's names, which the port's `CDarkSamusData` does not
carry yet.
