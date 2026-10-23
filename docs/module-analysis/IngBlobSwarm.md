# IngBlobSwarm REL (module 31): analysis notes

CIngBlobSwarm (IBSM): a CSwarmBasics boid swarm whose boids wind up an "into attack" pose, then
launch at the player and die on contact. The module's code is upstream's
`src/MetroidPrime/Enemies/CIngBlobSwarm.cpp` with the loader `SLdrIngBlobSwarm.inc`; the swarm
itself is CSwarmBasics (module 80, `SwarmBasics.md`). Retail asm:
`build/G2ME01/IngBlobSwarm/asm/MetroidPrime/Enemies/CIngBlobSwarm.s`.

## Survey (ibs1, 2026-10-23)

| function(s) | verdict |
|---|---|
| PreRenderBoid fn_31_A10 | **fixed**: retail tests `mSharedAttackBit` (0xA48 `lwz 0x5f4`) but clears the first attack slot's bit (0xA6C `lwz 0x5f8` mAttackModelBitBase, 0xA7C `andc`); upstream cleared the shared bit. Mirrored under TARGET_PC: the shared pose is re-cached per launched boid, slot 0 may keep last frame's pose |
| ctor fn_31_1EB8 | matches: anim id checks `addis/cmplwi 0xffff`, mAttackModelBitBase from 0x490 (`stw 0x5f8`), mSharedAttackBit = base + count (0x22B4), mSharedAttackActive cleared (0x2024, 0x618_24) |
| SLdrIngBlobSwarm ctor fn_31_5B8 | defaults match (inline in `SLdrIngBlobSwarm.hpp`, no marker) |
| Think fn_31_1744 | matches; retail does not count attackers inside the attack loop either (`cmplw r30` at 0x1A04 against the count loaded at 0x1884), so that loop exit is dead in retail too |
| AllocateSkinnedModels, UpdateSwarmAnimations, UpdateAllBoidMovement, LaunchBoid, KillBoid, collision callbacks, RenderBoid, ApplySteeringBehaviors, Load | scheduling only |
| UpdateBoid fn_31_B00 | matches; float association of the probe step is `scale*(dt*v)` in retail vs `(scale*dt)*v` upstream: kept (rounding only) |

## Shipped data (disc scan)

`tools/scan_ibsm_props.py` (about a minute): four IBSMs.

| area | layer | eid | attackers | proximity | timer | anims (into/attack) | maxHeight |
|---|---|---|---|---|---|---|---|
| Sanctuary 0b_cliff_hall_dark | 4 | e1033002b, e1033007c | 5 | 20 | 0.5 | 1 / 2 | 0 |
| Temple Grounds 0e_temple_hive_hall | 0 | 0x000c00d6, 0x000c00af | 1 | 0 (never attack) | - | 0 / 0 | 1 |

Every placement has valid anims and sets mVulnerableToSafeZone; no IBSM is near a safe zone.

## Coverage (ibs1)

Coverage build, driver `tools/cov/specs/ingblobswarm_cliff.con` (cliff L4, god mode, Activate at
f380, stands at the swarms, shoots, moves above/below, deletes e1033007c). Runs, all `--fast --frames 2600`:

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/swarmbasics.py <o1> \
      1baa96c2,0b_cliff_hall_dark --con tools/cov/specs/ingblobswarm_cliff.con --fast --frames 2600
    [COV_FORCE=timer|other] BUILD=... tools/cov/force.sh tools/cov/specs/ingblobswarm_force.py <o2> (same args)
    [COV_FORCE=novuln] COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=... tools/cov/force.sh \
      tools/cov/specs/ingblobswarm_safezone.py <o3> (same args)
    python3 tools/cov/gcov_report.py build-port-cov <o*>/gcov --module IngBlobSwarm

Marked (16, asm-checked, every live arm taken): Load D8, KillBoid 6A4, LaunchBoid 714,
BoidCollidedWithPlayer 8A8, BoidCollided 924, ApplySteeringBehaviors 98C, RenderBoid 9B8,
PreRenderBoid A10, UpdateBoid B00, UpdateAllBoidMovement 111C, UpdateSwarmAnimations 1230,
AllocateSkinnedModels 13F8, Think 1744, AcceptScriptMsg 1A60, dtor 1A80, ctor 1EB8.

| arm | how |
|---|---|
| Think: attackTimer expiry with slots exhausted, a winding-up boid touching the player | forced (`ingblobswarm_force.py`, COV_FORCE=timer: mAttackTimer 0) |
| Think: attackerCount 0 early return; KillBoid with a held slot | forced (COV_FORCE=other: mAttackerCount 0; mAttackSlot 0 on KillBoid entry) |
| UpdateBoid: safe-zone kill | forced (`ingblobswarm_safezone.py`: flag 1, PointIsInSafeZone true) |
| UpdateBoid: not vulnerable to safe zones | forced (COV_FORCE=novuln: flag 0) |
| ctor 42/57: invalid or -1 anim id | data-dead (every placement has valid anims) |
| ctor 41, 63: EH / `rs_new` null arms | compiler |
| Think 84:3/4 | static-init guard |
| Think 95:3/4: attackers reach the count inside the loop | dead in retail too (see survey) |
| AllocateSkinnedModels 133, UpdateSwarmAnimations 151:1/4 | shared attack model null: data-dead |
| UpdateAllBoidMovement 160/161 | !mAnimated: data-dead (always animated) |
| UpdateBoid 201:3/4: attacking, not launched, slot -1 | impossible: the slot is cleared only on launch |
| PreRenderBoid 236:1/2 | dead under the retail bug: the shared bit is never cleared |
| ApplySteeringBehaviors 272:1/2 | attacking boids never reach the base steering |
| everything else | natural |

asmcheck findings on the marked bodies are tool limits: CALL/XCALL to base-module (fn_80) callees and
inlined rstl/vector helpers (fn_31_26DC, 29F4, 2B10, 2C2C, 2350, 22F8, 1078, 152C, 15C8, 27EC, dtor
member dtors 1B44..1E0C), the loader's SLdr typedef readers, CALLS/BRANCH on the empty dtor and the
loader, NOPOS on CSwarmBasics' mAnimated/mVulnerableToSafeZone (shared header, no stated position).

Cliff `.con` shots miss the swarm (the known shoot-miss bug); KillBoid is reached through
collisions and the forced runs. Temple hive_hall (proximity 0) only reaches the non-attack paths.

Smoke family proposal: `ingblobswarm`, area `1baa96c2,0b_cliff_hall_dark`, driver
`tools/cov/specs/ingblobswarm_cliff.con`, `--fast --frames 2600`.
