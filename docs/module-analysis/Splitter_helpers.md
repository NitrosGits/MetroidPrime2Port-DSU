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

Of the 87 B rows, 30 carry a marker today and 57 do not; section 2 lists the 57. This lane
ports 8 of them (section 3) and marks the 4 whose every live arm ran.

## 2. The unmarked B rows

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

All 8 bodies are in `src/MetroidPrime/Enemies/PortCSplitterHelpers.cpp`; the declarations are
in the append-only helpers blocks at the end of the two classes.

| id | size | what | the class |
|---|---|---|---|
| `fn_75_6310` | 0x2C | the head's AddToRenderer: the base class's call only while the actor is alive | head |
| `fn_75_633C` | 0x2C | the head's PreRender: the same alive gate | head |
| `fn_75_6368` | 0x2C | the head's Render: the same alive gate | head |
| `fn_75_5E50` | 0x3C | the head's CanBeIngPossessed: only while it is docked (the chassis's id at 0xEFA) | head |
| `fn_75_5F18` | 0x74 | the head's Listen: alive, the noise type 4 and the distance (20 units) | head |
| `fn_75_E99C` | 0x48 | the chassis's CanBeIngPossessed: three of the chassis's own flags end it | chassis |
| `fn_75_E810` | 0x6C | the chassis's SetAttackTarget: it keeps a CIngSnatchingSwarm target's id | chassis |
| `fn_75_E90C` | 0x90 | the chassis's SetIngPossessed: it passes the possession on to the head it holds | chassis |

Eight stand-in call sites now call them instead of logging: the head's `PreRender`,
`AddToRenderer`, `Render`, `Listen` and `CanBeIngPossessed` (`PortCSplitterCommandModule.cpp`)
and the chassis's `CanBeIngPossessed`, `SetIngPossessed` and `SetAttackTarget`
(`PortCSplitterMainChassis.cpp`).

The port's stand-ins called the base class **unconditionally**, so the head drew and rendered
itself while it was dead and the head was possessable while it was docked: four of the eight
bodies are behaviour fixes, not just new code.

### 3.1 The run

**04_cliff_contraption** (world 1baa96c2, area 914f1381), **layer 2** - the fight with the
chassis u0532, its head u0531, the chassis u0521 and the head u0520. The coverage harness:

    tools/cov/run.sh tools/cov/specs/splitter_helpers_head.py /tmp/opencode/sph_cov_h3 \
      1baa96c2,914f1381 --driver "MP2_CONSOLE={port} LAYER=2 PLAN=Z python3 \
      tools/drivers/splitter_helpers_drv.py /tmp/opencode/sph_cov_h3/game.log 200 40" \
      --fast --frames 20000

The game exited with code 0 and the log has **0 `[port] caught` and 0 `reach-stub`** lines.

### 3.2 The arms

| id | the arms | how they ran |
|---|---|---|
| `fn_75_6310` (the head's AddToRenderer) | (a) the actor is alive: the base class's call runs, (b) it is not: nothing runs | **both** ran: 800 entries, the base class's `CPatterned::AddToRenderer` call 400 times (the `calls` key) and 400 entries without it. **Marked** |
| `fn_75_633C` (the head's PreRender) | the same two | **both** ran: 800 entries, `CPatterned::PreRender` 400 times. **Marked** |
| `fn_75_6368` (the head's Render) | the same two | **both** ran: 800 entries, `CPatterned::Render` 400 times. **Marked** |
| `fn_75_5F18` (the head's Listen) | (a) the actor is not alive: false, (b) the noise type is not 4: false, (c) the noise type is 4 and the distance is 20 or more: false, (d) the noise type is 4 and the noise is inside the radius: the flag byte at 0xF6A gets its 0x40 bit and the body returns true | (b) ran (the `noise 3` call, 2 entries), (c) ran (the `noise 4` call from 60 units up, 7 entries), (d) ran (the `noise 4` call next to the head, 1 entry, the return value 1). (a) did not: the head is never dead while the noise system still asks it. **Unmarked** (the alive arm needs a forced call) |
| `fn_75_5E50` (the head's CanBeIngPossessed) | (a) the head is docked: false, (b) it is not: the base class's answer | the body was never called in the run (the Ing possession system does not ask in this area): 0 entries. **Unmarked** |
| `fn_75_E99C` (the chassis's CanBeIngPossessed) | (a) none of the three flags is set: the base class's answer, (b) the byte at 0xD41's 0x80 bit: false, (c) the byte at 0xD40's 0x01 bit: false, (d) the byte at 0xD41's 0x40 bit: false | the body was never called in the run: 0 entries. **Unmarked** |
| `fn_75_E810` (the chassis's SetAttackTarget) | (a) the target is a CIngSnatchingSwarm: its id is kept at 0xCBC, (b) it is not: only the base class's flag byte changes | the body was never called in the run: 0 entries. **Unmarked** |
| `fn_75_E90C` (the chassis's SetIngPossessed) | (a) the chassis is not possessed: only the base class's call, (b) it is: the head it holds is told too | the body was never called in the run: 0 entries. **Unmarked** |

The four bodies that ran are marked; the other four have no caller in this area yet (the FSM
lane's states are still stand-ins, so nothing asks the chassis to be possessed or to take an
attack target), so they stay unmarked until a run calls them.

## 4. What is left, and why

- **53 B-class rows have no body.** They are the module's own helpers that the FSM lane's states
  and codes call (the beam effect's actor, the laser-pulse shot, the shield's effect, the
  collision actor managers) and the ones the two classes' constructors build. They are listed in
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
