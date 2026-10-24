# WispTentacle REL (module 86): analysis notes

CWispTentacle (WISP, in-game "Darkling Tentacle"): a wall/floor tentacle that sleeps, uncoils when the
player comes near (Spawn), patrols, lurks (Search), loop-attacks with contact damage, flinches and
withdraws when hurt, and can grab a Space Pirate (Grab/Pull). The module's code is upstream's
`src/MetroidPrime/Enemies/CWispTentacle.cpp` with the loader `SLdrWispTentacle.hpp/.inc`. Retail asm:
`build/G2ME01/WispTentacle/asm/MetroidPrime/Enemies/CWispTentacle.s`. Per-function table:
`WispTentacle_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (wisp1, 2026-10-23)

`tools/upstream_hollow.py WispTentacle`: nothing hollow. Every body was diffed against the retail asm
(registers and stack offsets normalized).

| function(s) | verdict |
|---|---|
| ctor fn_86_3628 | **fixed**: retail clears the knock-back burn bit, `mKnockBackController.EnableBurn(false)` (0x38B0..0x38C0, +0x681 bit 0x10); upstream called `SetPendingDeath(false)` |
| GetOrbitPosition fn_86_FD8 | **fixed**: `min_val(time, 1)` operand order (0x1038 lfs 1.0; 0x103C fcmpo f7,f1; 0x1040 bge); differs only for a NaN time |
| SLdrWispTentacle ctor fn_86_C1C | **fixed**: knock-back resistance 2.0 (0xCE4), ambient colour (1,1,1,1) (0xCF8), visor flags 0xF (0xCF0/0xCFC) |
| PreRender fn_86_1780 | retail stores an uninitialised stack word into model flags +0xfc; not reproducible, kept |
| UpdateGrabbedPirate, Think, AcceptScriptMsg | scheduling, fmadds and extra temporaries only |
| LoadWispTentacle fn_86_468 | differs only by the inlined SLdr ctor |
| Spawn/Withdraw/Flinch/Grab/Pull Update | retail inlines `CTryCommand::ShouldRetry` where the C++ calls `CAnimationState::CanIssueCommand`; same logic |

## Shipped data (disc scan)

31 placements: Temple Grounds 0t_temple_hall_dark L0 x6, Agon 0h_sand_hall_dark x4, Torvus
0l_swamphall_dark x4 and 0q_swamp_hall_dark x5, Sanctuary 02_cliff_reactorcore_dark L4 x2 and
07cliff_gyro_dark x10. None has a script connection in or out; all have spawnFromPortal 0 and
detectionHeight 0. Data-dead therefore: the AreaLoaded pirate/waypoint loop, SnapWaypointToFloor's
body, UpdateGrabbedPirate, DoUserAnimEvent's pirate arm, every portal arm (ctor, PreRender, the
BreakDodge step ternaries in Spawn/Withdraw) and IsPlayerNearby's height arm. The loader's
`!modelData` arm and GetAimPosition/Think's invalid-locator arms are model-data dead.

## Coverage (wisp1)

Coverage build, spec `tools/cov/specs/wisptentacle.py`, driver `tools/drivers/wisptentacle_hall.con`
(0t_temple_hall_dark, tentacle e00240036 at 8.4 -39.3 -33.4; god mode):

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/wisptentacle.py <out> 3bfa3eff,0t_temple_hall_dark \
      --con tools/drivers/wisptentacle_hall.con --fast --frames 6200
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module WispTentacle

Natural play: Sleep -> Spawn -> Patrol -> Search -> Attack (contact damage at 0.7 -39 -33.4, HitObject
-> ApplyDamage) -> Search; shots -> KnockBack -> Flinch -> Withdraw -> Sleep; `send Alert` -> Grab ->
Pull. Forced arms:

| arm | how |
|---|---|
| Update re-issue in Spawn, Withdraw, Flinch, Grab, Pull | forced (`statecall <id> <S> 0; statecall <id> <S> 1` in one tick) |
| Attack Update re-issue (body not in LoopAttack) | forced (`statecall <id> Attack 1` from Sleep) |
| Decrement in Spawn/Withdraw/Flinch/Attack/Search | forced (`statecall <id> <S> 0; send <id> Decrement`) |
| GetOrbitPosition Patrol, Attack after Patrol, Search after Patrol | forced (`statecall` + `orbit`) |
| ShouldSleep true/false | forced (`trigcall <id> ShouldSleep`, far and near) |
| KnockBack zero damage | `damage <id> 0` |

Marked (26): every state, trigger and query body plus Render, Touch, KnockBack, GetAim/OrbitPosition,
the dtor and the loader; remaining untaken arms are the data-dead ones above or the state switch's
default. Unmarked: ctor, AcceptScriptMsg (bomb Damage arm, AreaLoaded loop), Think (!GetActive),
PreRender (garbage word), UpdateGrabbedPirate, SnapWaypointToFloor, DoUserAnimEvent, the SLdr ctor/dtor
(header).

## Second pass (wisp2, 2026-10-24)

`tools/cov/specs/wisp2_force.py` + `tools/drivers/wisp2_hall.con` (the wisp1 driver plus
AIUpdateDisabled / Activate / HitObject / a real morph-ball bomb / Alert and the state calls), merged
with the wisp1 run (`gcov_report.py build-port-cov /tmp/opencode/wisp_{a,c,e}/gcov --module
WispTentacle`). Every write is one call, restored at the next entry of the same body.

| body | arms | forced | still open |
|---|---|---|---|
| PreRender 1780 | 2/2 -> **marked** | mSpawnFromPortal set (0x542's model-flag arm; no shipped placement spawns from a portal) | - |
| Think 13D4 | 15/17 | mActive clear (0x557 inactive arm), mPirateCaptured set (0x589, UpdateGrabbedPirate then returns at its own 0x479) | 575:1/2 and 584:1/2: the locator-not-found arms - `GetLocatorSegId(name).val() != 0xff` with the CSegId in a register at -O2; forcing the call's return (a `CAnimData::GetLocatorSegId` breakpoint + `gdb.FinishBreakpoint` writing $rax = 0xff from Think) was installed and the run finished, but the arms stayed untaken, so the value did not reach the test |
| AcceptScriptMsg 2A18 | 88/108 | `send HitObject` (0x222's cast arm) and `send AIUpdateDisabled`+`Activate` | 154/155/157/159 (the AreaLoaded loop: no shipped placement has a connection, and the list is load-time data), 166/176/177/194 (the Create body-controller and model-data arms), 211/242 (the ActorLights arms: mActorLights is not the auto_ptr the write assumed), 225/259/266 (the HitObject and Damage bomb tests: they need a bomb as the collision actor's last touched object, not a player), 253 (mCollisionManager null - nulling it crashes Think, which dereferences it) |
| UpdateGrabbedPirate 1B54 | 4/17 | mPirateCaptured set (the body now runs) | 479:1/2 (the body itself) and everything from 482 on: it needs a live CSpacePirate, and none of the six wisp areas ships one |
| ctor 3628 | 40/52 | - | 110:0/2 and the portal body 111-119 (mSpawnFromPortal at the ctor entry), 124:2/3 (the `time != Infinity` arm) and 127:1/2 (the `best <= FLT_EPSILON` arm), both read out of the animation database |
| SLdr ctor/dtor BA0/C1C | - | - | inline in `SLdrWispTentacle.hpp`: no standalone body to mark |

No retail divergence, no source change beyond the PreRender marker.
