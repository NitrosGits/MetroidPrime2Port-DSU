# Dark Samus helpers at 0xA000 and up (the helpers2 lane)

The slice: the unmarked class-B rows of `DarkSamus_functions.tsv` with an empty `state` column and
an offset at 0xA000 or above - 160 functions, the boss's helpers (the stand-ins, the path find's
helpers, the hint chain's helpers, the render helpers, the reflect path). The FSM handlers and the
triggers below 0xA000 belong to the other lanes.

## 1. The sort of the 160

| bucket | n | what it means |
| --- | --- | --- |
| ported (a body exists) | 51 | the port already has the body; mark it when every live arm ran |
| missing (no body) | 105 | port from the asm; 43 of them are called by the ported code, the other 62 hang under a stand-in or under a missing body |
| dead (no caller, no body) | 4 | fn_10_CEB4, fn_10_1BEFC, fn_10_1C7FC, fn_10_1EF38 - vtable slots the REL never fills |

The 51 ported ones are (REL fn -> the port's name): fn_10_A06C UpdateBoostBallAtActor,
fn_10_B458 FindArrivedHint, fn_10_B6C4 FindBoostBallHint, fn_10_B86C ActivateHintChains,
fn_10_BA38 BoostChainHasInactiveHint, fn_10_C104 SweepBeamDamage, fn_10_C2B0 UpdateSweepBeamAim,
fn_10_C470 UpdateSweepBeamEnd, fn_10_EFC8 IsInUnsafeSpot, fn_10_F40C GetScannableObjectInfo,
fn_10_F488 TakeDamage, fn_10_F5C0 GetTouchBounds (stand-in), fn_10_FF20 DamageVulnerability,
fn_10_100DC GetDamageVulnerability, fn_10_10210 GetAimPosition (stand-in),
fn_10_10358 GetOrbitPosition (stand-in), fn_10_11334 UpdateGlideEmitter,
fn_10_11974 UpdateScatterShotFire, fn_10_11C60 UpdateDiveFlight, fn_10_11DDC SendDiveAttack,
fn_10_11EB8 UpdateDiveStuck, fn_10_128D8 UpdateSafePosition, fn_10_12960 FindDiveLanding,
fn_10_12F60 FireScatterShotProjectile, fn_10_1444C IsLineOnMesh, fn_10_14A00 ApplyDashStuckBlast,
fn_10_14D14 UpdateDoubleDash, fn_10_14FE0 PickNextDash, fn_10_154FC PickDash,
fn_10_16B8C TurnTowardPoint, fn_10_1707C UpdateDodge, fn_10_17F04 AddToRenderer (stand-in),
fn_10_18964 Render (stand-in), fn_10_18AB8 PreRender (stand-in), fn_10_18D78 SetupStateMachine,
fn_10_19B64 CollectReachAttacks, fn_10_1AAC4 GetCollisionPrimitive (stand-in),
fn_10_1B1FC Think (stand-in), fn_10_1C6C8 ProjectileInfo, fn_10_1CDD0 BounceDive,
fn_10_1D860 UpdateAdditiveAnimation, fn_10_1D9A4 GetRepulsionPoint, fn_10_1DD14 Separate,
fn_10_1E178 AccumulateDamage, fn_10_1E26C PushAttacker, fn_10_1E2E8 EraseFirstAttacker,
fn_10_1E3C0 FindBestCoverPoint, fn_10_1E4C4 ScoreCoverPoint, fn_10_1E740 ShouldForgetLastAttacker,
fn_10_1E89C AcceptScriptMsg (stand-in), fn_10_1EE4C CanReflectHit.

Of the 105 missing ones, only these 20 are called by a body the port has (the rest wait for their
callers): fn_10_10010, fn_10_100D0, fn_10_13FFC, fn_10_16048, fn_10_17E48, fn_10_19014, fn_10_1AFB4,
fn_10_1B00C, fn_10_1B05C, fn_10_1B938, fn_10_1B9B0, fn_10_1C778, fn_10_1D794, fn_10_21F14,
fn_10_220EC, fn_10_22184, fn_10_B570, fn_10_B670, fn_10_EDB8, fn_10_FFB4.

## 2. What this slice marked

Two bodies, both proven arm by arm with a temporary probe (the probe is not committed):

| fn | the port's body | the arms |
| --- | --- | --- |
| fn_10_1D794 | `CDarkSamus::UpdateStillTimer` (0x0001D794, 0xC8 bytes) | **activate** (the timer cleared, the reference position set - the guest's argument r4 against 0, 0x0001D7B0), **still** (the timer grows by dt - the ble at 0x0001D80C), **moved** (the timer cleared, the reference position moved - 0x0001D810). All three ran. |
| fn_10_18D78 | `CDarkSamus::SetupStateMachine` (0x00018D78, 0x6C bytes) | no branch in the REL's body (the `InitializeStateMachine__10CPatternedFR13CStateManager` call at 0x00018D8C and the two table calls at 0x00018DAC/0x00018DCC); the body ran once, from the first Think. |

## 3. What is ported but still unmarked, and why

The probe (an entry log plus one log per arm) ran in the rooftop fight; these are the arms that
did **not** run:

- **fn_10_11334** `UpdateGlideEmitter`: the "no emitter" arms (the update after a deactivate, the
  deactivate without an emitter). They hang off the invalid sfx id (0xC48, the loader's argument,
  valid in every instance - proven by the four successful `AddEmitter` calls), so they are dead on
  shipped data except for an update that follows a deactivate - which the state machine's message
  order never produced.
- **fn_10_16B8C** `TurnTowardPoint`: the "the forward vector is not normalisable" arm (dead: the
  transform's forward is a unit vector) and the "the point is at the actor's own x/y" arm.
- **fn_10_1D860** `UpdateAdditiveAnimation`: the "no additive animation was found" arm and the
  "deactivate without one" arm (they come as a pair).
- **fn_10_1DD14** `Separate`: the "the contact time is fresh" arm (0x0001DE5C), the "the repulsion
  is strong" arm (0x0001DE78) and the "the separation is not normalisable" arm.
- **fn_10_1E178** `AccumulateDamage`: the "the health is gone and it counts" arm (the health has to
  drop **after** the activate; in the run the health changed twice, both times with the activate
  message).
- **fn_10_1E26C** `PushAttacker`: the "the uid is invalid" arm (the caller hands it the uid it read
  from 0xC24, which is invalid until an attacker is known - the port's caller skips the call then).
- **fn_10_1E2E8** `EraseFirstAttacker`: the REL's body shifts the uid list down in a loop with two
  bounds checks (0x0001E2F0/0x0001E308); the port's body only does the two-entry case.
- **fn_10_100DC** `GetDamageVulnerability`: the enrage arm (the phazon states).

## 4. What this slice ported

`src/MetroidPrime/Enemies/PortCDarkSamusHelpers2.cpp` (new) holds **fn_10_17E48** (0x00017E48,
0x8 bytes): the stage count - the guest's 0x978, one load (`lwz` at 0x00017E48) and a `blr`. Its
only caller is fn_10_FD4C (`WatchStageChange`), which compares the count with the one it saw last
(0xD54). The port's fn_10_FD4C reads the member directly, so the new body does not run yet and
carries no marker; it will when the caller uses it.

The other cheap ones of the 105, for the next slice, with what they need:

- fn_10_100D0 (0xC) and fn_10_10010/220EC/22184: the damage-vulnerability container (a 12-byte
  entry list) - the host's `CDamageVulnerability` has no such container yet.
- fn_10_EDB8 (0x14): `lbl_10_rodata_3BC[index]`, a float table; its caller fn_10_153EC is the
  SideDash state, which the FSM2 has no node for.
- fn_10_19014 (0x48): sets two bits of the byte at 0x34C (a base-class member the port does not
  map) and calls `RemoveMaterial(0x25, mgr)`.
- fn_10_1B00C/1B05C/1B938/1B9B0: the reflect path (fn_10_1B9B0 is the note's "still unmarked on
  purpose" one).
- fn_10_B570/B670: the hint chain's helpers - they free a chain object the port does not have.
- fn_10_1C778 (0x84): the path find's destination (0x8B4/0x8B8/0x8BC) - the note says it is
  ported; the port's body is missing.

## 5. The run

Sanctuary `10_cliff_rooftop` (world `1baa96c2`), layer 2 (`MP2_INPUT="LAYER2@400"`), boss `u04f7`,
5600 game updates at `MP2_TICKS_PER_FRAME=8`: `send u04f7 Activate`, `place u04f7 player 3`, 67
`press Y 45` (the missiles), `damage u04f7 130` x3, then the states `BoostBallInto`, `Taunt`,
`TravelToCoverPoints`, `Dash`, `DiveAttack`, `MissileJump` forced one after the other. 0 caught,
0 reach-stub, exit 0. The probe counted: UpdateStillTimer 178 entries (4 activate, 123 still, 51
moved), SetupStateMachine 1, and the arms of the eight other probed bodies as listed above.


## 6. The coverage run (tools/cov, no probes)

`tools/cov/specs/darksamus_helpers2.py` covers the 51 ported bodies of this slice plus the five
ids the DarkSamus2 lane dropped; the labels carry the retail offsets. The driver
`tools/cov/specs/darksamus_rooftop.con` is the rooftop fight (Sanctuary 10_cliff_rooftop, layer 2,
boss u04f7: activate, the player next to it, 67 missiles, four direct damages, then the states
BoostBallInto, Taunt, TravelToCoverPoints, Dash, DiveAttack, MissileJump, SweepBeam, ScatterShot,
MovingScatterShot, MeleeAttack and HeavyHit). The run: 6400 updates, 0 caught, 0 reach-stub.

The arms the fight cannot reach were forced with `tools/cov/force.sh` and
`tools/cov/specs/darksamus_helpers2_force.py` (the gdb side is `tools/cov/force_ds2.py`: it writes
the members at the entry of the body and logs what it forced). Those arms are **forced**.

| fn | the port's body | the arms |
| --- | --- | --- |
| fn_10_11334 | UpdateGlideEmitter | the activate adds the emitter (3), the update with the emitter (164), the update without an emitter (2, **forced**), the deactivate with the emitter (3), the deactivate without an emitter (2, **forced**); the "the sfx id is invalid" arm is dead on shipped data (the loader's id, the four AddEmitter calls) |
| fn_10_11974 | UpdateScatterShotFire | not firing (152), firing (179); the "the rate is zero" arm is dead on shipped data (the rate comes from the loader's mScatterShotProjectilesPerSecond and is above zero in every hit) |
| fn_10_128D8 | UpdateSafePosition | the flag is set (4), the flag is clear (1: the unsafe-spot test answered false) |
| fn_10_14FE0 | PickNextDash | no dash to take (4), the actor stood still (**forced**), the dash is stuck (**forced**) |
| fn_10_1A554 | CollectFadeAttacks | not fading (259), fading out (266, **forced**) |
| fn_10_1E178 | AccumulateDamage | the health is the same (135), the health changed and it does not count (2), the health changed and it counts (**forced**), the damage decays below zero (133, the earlier probe run) |
| fn_10_1E26C | PushAttacker | the uid is invalid (**forced**), the list has two entries (5), the list has room (2) |
| fn_10_16418 | IsPlayerAimLineClear | the aim line is clear (20), the aim line is blocked (4) |
| fn_10_164C0 | IsGunLineClear | the ray found nothing, the ray found something (the caller fn_10_16418 answered false four times, and it is the same test) |

Dropped again: **fn_10_16B1C** (IsOnPath) - the coverage run saw only the "on the path" outcome
(82 hits, 82 returns), so the provisional marker of the DarkSamus2 lane does not stand.
**fn_10_1905C** (ClassifyLineToPlayer) keeps its marker: the return-value data of that run is
incomplete for it (275 of 3315 hits), so the re-check could not disprove the earlier lane's
evidence (DarkSamus.md, slice 23).


## 7. The cov tool's two misfires, fixed

Both were in `tools/cov/gdb_cov.py` (the fix is generic ✓).

- **`calls` on the big bodies.** The demangled name of a function's `.cold` part is the function's
  own name (only the *mangled* name carries the `.cold`), so the filter (which looked at the
  demangled name) let the `.cold` symbol in. Its block ends at the end of the whole function, so
  the disassembly ran past the body and the call sites landed in other code - the table then
  listed the whole game's calls. The fix: filter the `.cold` and the clones on the **mangled**
  name as well, and clamp the disassembly to the symbol's own extent (the next symbol's
  address). Checked against the asm: fn_10_12F60's table now lists only the calls its body makes.
- **`ret` losing lines.** The hit counter of an entry mixed three things: the entry hits, the
  call-site records and the return-value records, and for a `ret` entry the entry hit was not
  counted at all (the counter only saw the return records) - so a table row said "82 hits" for
  41 calls. The fix: only the entry breakpoint counts as a hit; the call-site and the
  return-value records are arms of that entry (they keep their own keys), so the row now says
  how many times the body ran and the arms below it say what it answered.

`tools/cov/force.sh` is generic now (the gdb side is `$COV_GDB_SCRIPT`, default
`tools/cov/force.py`; the module's class and the forced members live in the spec), and
`docs/LANE_BRIEF.md` ("Coverage without probes") says so.


## 8. The arms re-measured with the fixed tool

`tools/cov/specs/darksamus_helpers2_arms.py` (the fourteen open bodies plus fn_10_12F60, a big
one, to check the call sites). With the fixed tool the row's hit count is the number of times the
body ran and the arms are the keys under it.

- **fn_10_16B1C** (IsOnPath): both arms now - 47 on the path, 21 off it (the earlier run had
  lost the return records, so the marker had been dropped in the last slice; it stands again).
- The others still miss an arm: 1DD14 (the "the contact time is fresh" arm, the strength and
  the "not normalisable" arms), 16B8C (the "the point is at the actor's own x/y" arm), 1D860
  (the "no additive animation" arm), 1E740 (the waypoint and the cover-point arms), EFC8 (the
  "the safe position is set" arm), 1444C (the false outcome), 1E3C0 and 1C6C8 (the null
  outcome), 1EE4C (the true outcome), 14D14 (the GetAnimOver arm), 19B64 (the mask bits
  0x20/0x4/0x80), 154FC (most of its arms) and 1905C (every return but zero). Their arms depend
  on locals, so the entry keys cannot show them: they need a 'loc:' breakpoint per arm line.
- The call sites are the body's own now: fn_10_12F60's table lists only what its body calls
  (GetScatterShotAimPoint, LookAt, ObjectById, the cast, UpdateCurrentAttack, TurnTowardPlayer,
  the collision failsafe ...), checked against the asm.


## 9. The per-arm measurement ('loc:' breakpoints)

`tools/cov/specs/darksamus_helpers2_loc.py` puts a 'loc:' breakpoint on every arm line of the
thirteen open bodies (the lines of the source the binary was built from) and counts the call
sites of each body as well. The run: the rooftop fight, 6400 updates, 0 caught, 0 reach-stub.

**What the two kinds of measurement are worth**: a call site is exact (each call site is its own
code), a line breakpoint is not - at -O2 the compiler merges the code and gdb moves the
breakpoint to the nearest line that has code ("No compiled code for line 613 in file
PortCDarkSamus.cpp" for the lines the switch of fn_10_1C6C8 ended up in). So the call sites are
the evidence, the line breakpoints are a hint.

**Marked**: **fn_10_14D14** (UpdateDoubleDash) - every arm ran (the activate 3, the deactivate 3,
the step command 108 with the dash blocked once, the next dash step 1).

**Still open, with the arm that is missing**:

| fn | the missing arm | why it is hard |
| --- | --- | --- |
| fn_10_1DD14 | the contact time is fresh, the weak/strong repulsion, the separation is not normalisable | the strength and the separation are locals |
| fn_10_16B8C | the forward is not normalisable, the point is not normalisable | the flat forward is zero only when the actor looks straight up; the point is at the actor's own x/y |
| fn_10_1D860 | no additive animation was found, deactivate without one | the first is the random pick of FindBestAnimation |
| fn_10_1E740 | the path find is over, the attacker has no cover point, the distance test | the waypoint count and the cover point are locals |
| fn_10_EFC8 | the hint is skipped, the hint is in the radius, no hint is close | the shipped fight script has no type-0x1C hint in the radius |
| fn_10_1444C | the height is out of the band, the line is on the mesh | the mesh height under the point |
| fn_10_1E3C0 | no cover point was better | the cover points of the fight script |
| fn_10_1C6C8 | the second scatter info, the scatter info, no info | the switch has no line info at -O2 |
| fn_10_1EE4C | the hit is too recent, the weapon is not a missile, there is no stage, the radius damage | the merged returns hide which early out fired |
| fn_10_19B64 | the bits 0x100, 0x4, 0x80, the player cannot be walked to, the bit 0x20, the bit 0x10 | the attack bits come from the stage's shipped data |
| fn_10_1905C | the direction is not normalisable, no hit, the hit is nearer, the hit is farther | the ray and the distances |
| fn_10_154FC | most of its arms | the path clearance and the slots |

**The forced calls** (`tools/cov/specs/darksamus_helpers2_loc_force.py`, one per arm that a
member write can reach). The forced run (the same fight, the members written at the entry):
- **fn_10_1DD14**: the contact-time arm - the key "the contact time is fresh" 181 (forced) against
  1 natural. The arm ran, but the strength arm is still unmeasured (a local), so the body stays
  unmarked.
- **fn_10_1D860**: the deactivate-without-an-animation arm - the key (the message is not the
  activate, the animation id is -1) 189 (forced). The other arm of the body ("no additive
  animation was found") is still unmeasured (the random pick), so the body stays unmarked.
- **fn_10_1C6C8**: the second-scatter-info arm was forced (the flag), but the switch has no line
  info at -O2, so there is nothing to read the arm from: the body stays unmarked.

The rest of the missing arms need the world (the mesh, the ray, the cover points) or a call with
different arguments, so they stay open.

**The merged profile** (the natural run and the forced one, `tools/cov/gcov_report.py build-port-cov
<run1>/gcov <force>/gcov --module DarkSamus`) shows what the force closed: fn_10_1E740's "the
attacker has no cover point" (only the "the path is shagged" is left), fn_10_1DD14's "the contact
time is fresh" (the strength and the "the separation is not normalisable" are left),
fn_10_1D860's "deactivate without an animation" (the "no additive animation was found" is left)
and fn_10_1EE4C's "the hit is too recent" (the weapon type, the stage and the radius damage are
left). fn_10_1C6C8's force reached the scatter shot's case but the arm it opens (the optional
that holds the info) is never cached, so it stays open; fn_10_154FC's fight covered four of its
thirteen arms on its own (the fight is not the same every run). No body of the twelve is
ALL-ARMS yet, so no new marker was added.


## 10. The gcov branch coverage (the exact arms)

A coverage build (`-B build-port-cov -DMP_PORT_COVERAGE=ON`, the same cmake line as
build-port-link plus the option) compiles the port's REL module bodies with gcov, so one ordinary
run reports the branch coverage of every body of the module at once, and the branch counts are
exact at -O2 (unlike the line breakpoints of the 'loc:' measurement of §9).

The run: the rooftop fight (tools/cov/specs/darksamus_rooftop.con), 6400 updates, 0 caught, 0
reach-stub; the report: `tools/cov/gcov_report.py build-port-cov <run>/gcov --module DarkSamus`.

**All-arms**: fn_10_14D14 (UpdateDoubleDash) - the marker of the last slice stands (10/10
branches, 19/19 lines).

**The twelve open bodies, with the exact untaken branch** (the line of the branch whose direction
never happened):

| fn | the untaken branches | the arm |
| --- | --- | --- |
| fn_10_1C6C8 | 609, 613, 615 | the switch: the scatter shot's info, the second info, no info |
| fn_10_1D860 | 855, 860 | no additive animation was found; deactivate without one |
| fn_10_1DD14 | 819, 821, 824, 829 | the point is zero; the contact time is fresh; the repulsion is weak; the separation is not normalisable |
| fn_10_16B8C | 4169, 4174 | the forward is not normalisable; the point is not normalisable |
| fn_10_1905C | 3606, 3622, 3624, 3629, 3632 | the direction is not normalisable; the ray hit; the hit is nearer than the player minus eight; no hit; the hit is nearer than the player |
| fn_10_19B64 | 3927, 3930, 3939, 3943, 3947, 3951, 3955, 3969 | the attack bits 0x1, 0x100, 0x20 (the ring path), 0x4, 0x80, 0x10 and the two inner tests |
| fn_10_1E3C0 | 629, 630 | the cover point is skipped (the null, the inactive and the wrong area) |
| fn_10_1E740 | 710, 718 | the path is shagged; the attacker has no cover point |
| fn_10_1EE4C | 2204, 2207, 2211, 2215 | the hit is too recent; the weapon is not a missile; there is no stage; the radius damage |
| fn_10_1444C | 3892, 3896 | the height query failed; the height is out of the band |
| fn_10_EFC8 | 876, 882, 883, 890 | the safe position is too low; the hint is skipped; the hint is of another type; the hint is in the radius |
| fn_10_154FC | 4554, 4565, 4573, 4575, 4612, 4619, 4620, 4628, 4636, 4640, 4643, 4645, 4661 | the facing, the slots, the clearance and the end distances |

**Every one of these untaken branches is a retail arm** (checked against the REL's asm): the two
CanBeNormalized tests of fn_10_16B8C are the retail's (the `bl CanBeNormalized` at 0x00016BF4 and
0x00016C50, each with its `bne` over the Normalize), and the four tests of fn_10_1DD14 are the
retail's (the point's three fcmpu at 0x0001DD8C..0x0001DDB4, the contact time at 0x0001DE5C, the
strength at 0x0001DE78 and the separation's CanBeNormalized at 0x0001DED4). So no body of the
twelve is marked because of an edge the retail does not have.

**The forced calls** (`tools/cov/specs/darksamus_helpers2_gcov_force.py`, one per arm a member
write can reach; the run's gcov counters are merged with the natural run's by the report tool):
fn_10_1DD14's contact-time arm, fn_10_1D860's deactivate-without-an-animation arm, fn_10_1E740's
attacker-has-no-cover-point arm, fn_10_1EE4C's too-recent and no-stage arms and fn_10_1C6C8's
second-scatter-info arm. The arms that need the world (the mesh, the ray, the cover points, the
stage's attack bits) or an argument the caller does not pass (the weapon type, the damage info)
stay open.

**The merged profile** (the natural run and the forced one, `tools/cov/gcov_report.py build-port-cov
<run1>/gcov <force>/gcov --module DarkSamus`) shows what the force closed: fn_10_1E740's "the
attacker has no cover point" (only the "the path is shagged" is left), fn_10_1DD14's "the contact
time is fresh" (the strength and the "the separation is not normalisable" are left),
fn_10_1D860's "deactivate without an animation" (the "no additive animation was found" is left)
and fn_10_1EE4C's "the hit is too recent" (the weapon type, the stage and the radius damage are
left). fn_10_1C6C8's force reached the scatter shot's case but the arm it opens (the optional
that holds the info) is never cached, so it stays open; fn_10_154FC's fight covered four of its
thirteen arms on its own (the fight is not the same every run). No body of the twelve is
ALL-ARMS yet, so no new marker was added.
