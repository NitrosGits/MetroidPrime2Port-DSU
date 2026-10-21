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
| `fn_10_2EFC` | 0x8 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_2F04` | 0xC | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
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
| `fn_10_452C` | 0xCC | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_45F8` | 0x270 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_4868` | 0x60 | method | 4 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_48C8` | 0x44 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
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
| `fn_10_5974` | 0x7A4 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_6118` | 0x19C | method | 1 | ported body | the anim event's melee/missile arm: the attacks have no ProjectileInfo in the port yet |
| `fn_10_6374` | 0xEC | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_65CC` | 0x1EC | method | 4 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_69C0` | 0x80 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_6DAC` | 0x8C | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_6F90` | 0xC4 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_7304` | 0x168 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_746C` | 0xCC | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_7538` | 0xD8 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_7610` | 0x170 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_7780` | 0x38 | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_77B8` | 0xB4 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_786C` | 0x1A8 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_7D00` | 0x298 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_7F98` | 0x17C | method | 3 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_812C` | 0x4C | method | 3 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_8178` | 0x5C | method | 1 | ported body | the anim event's melee/missile arm: the attacks have no ProjectileInfo in the port yet |
| `fn_10_8698` | 0xCC | method | 7 | ported body | the taunt command; the port runs it inline in the states (the lane's half) |
| `fn_10_8764` | 0x10C | method | 11 | ported body | the melee command over a state's messages; the port runs it inline (the lane's half) |
| `fn_10_8870` | 0xEC | method | 4 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_895C` | 0x3D4 | method | 1 | declared only | the boost ball sub-object's constructor; the host keeps two of its members |
| `fn_10_8D30` | 0x84 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_8DB4` | 0x19C | method | 1 | cited | the sweep beam sub-object's constructor; the host keeps two of its members |
| `fn_10_8F50` | 0x214 | method | 1 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_9164` | 0x1B0 | method | 1 | cited | the anim event's melee/missile arm: the attacks have no ProjectileInfo in the port yet |
| `fn_10_9624` | 0x20 | method | 6 | ported body | ported body in PortCDarkSamusHelpers.cpp; no caller in the port yet |
| `fn_10_9644` | 0xE0 | method | 2 | ported body | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_9920` | 0x4C | method | 2 | ported body | **marked** - both arms (the null one naturally, the actor one forced) |
| `fn_10_996C` | 0x4C | method | 4 | missing | missing body (the caller is a state or a helper the port does not have yet) |
| `fn_10_99B8` | 0x10C | method | 3 | ported body | stops the boost ball's particle effects: the host has no such effects |
| `fn_10_9AC4` | 0x60 | method | 4 | ported body | **marked** - both arms (the null one naturally, the delete one forced) |
| `fn_10_9B24` | 0x2DC | method | 1 | ported body | builds the boost ball's particle effects: the host has no such effects |
| `fn_10_9E00` | 0x26C | method | 2 | missing | missing body (the caller is a state or a helper the port does not have yet) |

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
