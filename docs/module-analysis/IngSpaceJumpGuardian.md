# IngSpaceJumpGuardian REL (module 34): analysis notes

CIngSpaceJumpGuardian (in-game "Jump Guardian", the Ing guardian of the Space Jump Boots): jumps
between CScriptAiJumpPoints, fires mini-portal plasma beams, taunts, spawns shock waves on landing and
a blob effect. Code is upstream's `src/MetroidPrime/Enemies/CIngSpaceJumpGuardian.cpp`, loader
`include/MetroidPrime/ScriptLoader/SLdrIngSpaceJumpGuardian.hpp`. Retail asm:
`build/G2ME01/IngSpaceJumpGuardian/asm/MetroidPrime/Enemies/CIngSpaceJumpGuardian.s`. Per-function
table: `IngSpaceJumpGuardian_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (sjg1, 2026-10-23)

Every sub-100% body diffed against retail (registers/stack normalized, rodata by value).

| function(s) | verdict |
|---|---|
| SelectJumpTarget, FilterJumpPoints | **fixed**: jump point is read through `GetJumpPoint` at +0x180, upstream used the wrong member |
| CollisionDamage | **fixed**: weapon direction is the forward column; knock-back gated on mDamageable bit 0x08 |
| Jump OffGround/Launching | **fixed**: `SetMomentumWR` (retail), not a velocity set |
| DoUserAnimEvent -> SpawnShockWave | **fixed**: retail passes the event's locator name; TARGET_PC overload `SpawnShockWave(CStateManager&, const rstl::string&)` placed after the original so the strip-diff stays exact |
| ComputeJumpVelocity | **fixed**: apex height `startZ < targetZ ? targetZ : startZ` |
| CameraShake | **fixed**: `FindConnectedObject` (retail) instead of `GetConnectedObject` |
| SpawnBlobEffect | **fixed**: `effect->SetNextDrawNode(GetUniqueId())` (0x112C `sth 0x12a`) |
| SLdrIngSpaceJumpGuardian ctor (fn_34_B7C) | **fixed**: ambient colour white, visor flags 0xF, sound/sound_MiniPortal -1, beam inner CColor(0.5,0.5,0.5,0.5), outer (0.6,0.6,0,0.5) |
| ctor 47%, Jump 82%, FindBestMiniPortals 86%, Load 73%, CreateLight, FindJumpPoints, UpdateBlob, GetAimPosition | equivalent: material masks 30/31/49/32/43 and portal masks 33/32/47/20/59 match, strings match, CreateLight light-id order and push_back growth equivalent |
| SpawnBlobEffect / Dead token | retail passes `TToken`, upstream a `TLockedToken`; kept (no behaviour difference seen) |

## Shipped data

Guardian u051f (editor 001900e9) in Agon `42b935e4,02_sand_sandfalls_dark` (area 25), layer 2,
inactive until Activate. HP 1100, dies to about 12 shots; script connections delete it after death.

## Coverage (sjg1)

Coverage build, spec `tools/cov/specs/ingspacejumpguardian.py`, drivers
`tools/drivers/ingspacejumpguardian_sandfalls.con` (fight to death) and
`..._sandfalls_forced.con` (forced calls, then fight), god mode:

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/ingspacejumpguardian.py <out> \
      42b935e4,02_sand_sandfalls_dark --con tools/drivers/ingspacejumpguardian_sandfalls.con --fast --frames 6000
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module IngSpaceJumpGuardian

| arm | how |
|---|---|
| Listen noise arms | forced (`noise 0/4/6 [x y z]`) |
| PathOver, ShouldTaunt, PathShagged, IsFacing* | forced (`trigcall u051f <name>`) |
| FindBestMiniPortals, SelectAttackAction, SelectJumpTarget, SetFace*/SetIntroJumpTarget | forced (`codecall u051f <name>`) |
| Taunt/MiniPortalAttack/FaceTarget/SteerToDest/PathFind/Jump msgs 0/1/2 | forced (`statecall u051f <state> <msg>`) |
| intro pattern, jumps, shock waves, beams, blob, death | natural (`send u051f Activate`, shoot loop) |

Marked (30): Think, Listen, ShouldJump, PathOver, FindBestMiniPortals, SetupCollision, CreateLight,
GetCurrentStruct, SpawnBlobEffect, PreRender, AddToRenderer, GetDamageVulnerability,
SetupStateMachine, StateOver, HasIntroPattern, IntroPatternOver, HasJumpTarget, ShouldTaunt,
ShouldMiniPortalAttack, FoundMiniPortalAttack, SkipToNextJumpPoint, PathExists, Start,
EnableEnergyBar, SelectTarget, SetIntroJumpTarget, SetFaceJumpTarget/AttackTarget/Waypoint,
SpawnShockWave (on the PC overload).

Unmarked, with the untaken arms: ctor (EH/data arms 159-194), dtor (deleting arm), AcceptScriptMsg
(212/218/224), DoUserAnimEvent (effect/eyes event cases), Render, GetAimPosition, IsFacingTarget/
IsFacingWaypoint (cast fail), PathShagged, Dead (503-514 never run: the actor is deleted by script
first), FollowIntroPattern, FaceTarget/Jump/Taunt/MiniPortalAttack/PathFind/SteerToDest sub-arms,
SelectJumpTarget, SelectAttackAction (776), ComputeJumpVelocity, TouchDamage, CollisionDamage,
UpdateCollisionVulnerabilities, SetCollisionVulnerability, UpdateLight, CameraShake (null shaker),
FilterJumpPoints, FindJumpPoints, UpdateBlob, Load (null-model arm), the SLdr ctor/dtor (header),
GetSearchPath/IsListening/GetGravityConstant (not instrumented).

## sjg2 checkpoint: loader/data/constructor (2026-10-24)

Basee32d198ab. Eleven further bodies certified, **40 → 51/69 behaviour bodies**.
No new retail mismatch found; production diff is marker-only. Full listings for all29
previously unmarked bodies and checkpoint evidence are in `/tmp/opencode/reports/sjg2-evidence/`.
The TSV's marker column is stale; remaining inventory below is compared against actual
source/header `// @ fn_34_...` markers.

### Full instruction/block audit

| body / retail blocks | correspondence |
|---|---|
| GetSearchPath0–4 | Return member+8D0; host returns &mPathFindSearch. |
| IsListening8–C | Returntrue. |
| Gravity10–18 | Rodata0=60, returnfloat. |
| Stage defaultsE84–ED0 | Stores at0/4/8/C/10:10,90,33.299999,1,1500. Rodata80/98/9C/74/A0 independently decoded using asmcheck.rod_value. |
| Stage readerD18–E44 | Unsigned16 count; five hash cases A77F6212,7EAF8D70,03698C10,FD46B85C,3E370622 read float to corresponding0/4/8/C/10. Unknown skips propertySize bytes; full count loop/epilogue. |
| Data defaultsB7C–BEC | BlobFFFFFFFF at0, sound-1 at4; four stage default ctors at8/1C/30/44; initial green at58; portalFFFFFFFF at60, portalSound-1 at64; damage/beam/shock-wave member defaults at68/78/C0. |
| BEC–D14 | Override light white at58, attenuation5 at5C; damage weapon11/damage20/knockback10 at68/6C/74; beam unknown0 at78 (also supplied by its shared default ctor), length500 at90, expansion4 at98, life1 at9C, pulse20 atA0, shutdown.25 atA4, pulseEffectScale2 atAC; colors inner(.5,.5,.5,.5) atB4, outer(.6,.6,0,.5) atB8. Existing PC correction supplies sounds-1 and correct color conversion. Rest of member defaults unchanged. |
| Data reader864–ADC | Unsigned16 count; thirteen cases map exactly: blobCC5A4918→0, sound46E902E8→4; four stages5E1D1931/6B08E2E5/F223AA76/D0DB5F7A→8/1C/30/44; colorBD3EFE7D→58; attenuationD24B888F→5C; portalA926F8A8→60; portalSound4051FD1A→64; damage424A6D37→68; beam9C170968→78; shock8F4787CB→C0. Default ReadBytes(nullptr,size), loop/epilogue. |
| Data destructorADC–B78 | Reverse member destruction: shockC0,beam78,damage68,stages44/30/1C/8; nullable retail wrapper then signed-short deletion flag>0 frees. Host nonvirtual dtor runs the member cleanup; caller owns/free handles heap deletion. |
| Loader destructor7E8–860 | Reverse members: data2F8,actor280,patterned3C,editor0; same nullable/deleting MW wrapper policy. Host has no legal null-this destructor call. |
| Loader170–23C | Editor/patterned/actor/data default ctors. Override turn360 at444,minAttack0 at454,health150 at484,resistance2 at488,creatureSize1 at668,white ambient694,visor flags15 at6E4. Existing PC header corrections already supply the ambient/visor stores. |
| 240–2FC | Four property cases: editor255A4580→400,patternedB3774750→43C,actor7E397FED→680,dataF9837AB3→6F8. Unknown skip and count loop. |
| 300–38C | LdrToModelData(scale428,modelFFFFFFFF,animation610,true); optional copy4A0. Invalid optional returnsnull after data/actor/patterned/editor destruction. Redundant invalid-optional destructor test following known-invalid is dead. |
| 390–49C | Native-size allocation (retailA38). Constructor args r3=allocation,r4=&uid14,r5=&name400,r6=converted entity-info,r7=&xf1C,r8=&modelFC,r9=&actor4C,r10=&patterned14C,stack08=&data6F8. Patterned conversion43C/null possession, actor680, editor400; uid allocated by mgr. Invoke5E58, then patterned/model/data/actor/patterned-loader/editor cleanup; returnactor. |
| Guardian ctor5E58–5EBC | CPatterned(AI1A,uid,name,flavor0,info,xf,model,patterned,movement0,collider1,body1,actor). The loader's actor/patterned argument order was checked against this register shuffle, not inferred from names. |
| 5EC0–5F5C | Derived vtable; copydata63D0 to7C0. Convert data+68 damage and copy all damage/flags/sfx fields to8B0. Null collision manager; path search(area0,flags301,pathfindingIndex,1,1,avoidance0,rootCenter0). |
| 5F60–6000 | Default speed=base speed3C8; deathTimer2; lightidinvalid/intensity0; head/collarFF; attackAction-1; player/face/intro/lastJump/blob/jump idsinvalid; apex0,mode-1,jump/shield/duration0,sfx handle0. |
| 6004–6168 | Portal effect data+60==FFFFFFFF selects empty optional, otherwise PART pool lookup at slotC, GetObj and locked-token construction. Copy optional to9F8; release initialized temporaries only. Stack/embedded placement-new null checks cannot be true for live native objects; host throwing-new/token ownership ABI supplies equivalent normal paths. |
| 616C–61E0 | PortalCount0,index-1. Byte+A34: jumping80/hasPath40/heardNoise20/lastWaypoint10 clear,damageable08 set,blobActive04 clear,render02 set. |
| 61E4–6290 | Lookup head and Collar_SDK locators, store segments; disable knockback physics, disable reactions2(KnockBack)/3(Hurled), shadowfalse, returnthis. |

### Executed arms and oracles

`tools/cov/specs/sjg2_load_arms.py`, driver `tools/drivers/sjg2.con`, real room/layer2.
Capture the natural four-property2738-byte stream and original info header. Replays use
real malloc-backed CInputStream objects; the raw info copy's connections vector is emptied
before use so no expired nested storage is borrowed. All fixture constructors/destructors,
pool/token/model callees and reader bodies are real; no callee is stubbed or skipped.

| fixture | arms reached |
|---|---|
| shipped+unknown | Top-level, data-level and all four stage readers append unknown12345678,size3,"xyz"; every real default ReadBytes arm runs. Stage0 additionally reads a duplicate empty stage payload, covering D18 count0 without changing already-read fields. All known property cases retain shipped bytes. |
| empty data+unknown | Top-level F9837AB3 with count0 data plus an unknown property:864 count0; naturally invalid default model, real null return, full loader/member cleanup. |
| absent effect | Same valid shipped model/data, except portalEffectFFFFFFFF: real guardian ctor's empty optional arm; fixture is destroyed before registration/gameplay. |
| empty top | count0 stream: top loop skips; default model invalid returnsnull. |
| heap data/top fixtures | Construct/destroy real default SLdrData and SLdr loader on heap, then free their storage, covering host caller-side deletion corresponding to MW's deleting wrapper. Direct GDB C1/C2-alias calls can unwind to a dummy frame before the prologue, so only these two extra fixtures omit the finish oracle; the five natural/replay data constructors assert the same defaults. |
| getters | Explicit assertions: IsListeningtrue,gravity60,search pointer equals member address. |

Finish oracles assert all five defaults of each of the four stage structs; sounds/assets,
attenuation, projectile damage, six beam scalars, all color channels; guardian flags, ids,
mode/action/count/index, timers, path flags and optional presence. Color expectations are
white255x4,inner127x4,outer153/153/0/127. Guardian constructor runs twice with effect and
once without. The final run has **15 PASS** (five data-default oracles,three guardian-default
oracles,seven suite assertions),seven successful inferior calls,zero force/Python errors.

| body | gcov coverage |
|---|---|
| Guardian5E58 | 44/45 lines,22/24 nonthrow edges. Only missing source line186 is EH block26, reached solely via throwing edge9→25 then25→26→27 (saved gcno CFG); normal token operations at187 execute twice. Missing edges33→34/36 are optional cleanup reached solely on exceptions. Thus every live retail arm ran. |
| Loader170 | 10/10 lines,10/10 edges ALL-ARMS. |
| Data/header aggregate | 124/124 lines,62/62 nonthrow edges ALL-ARMS; full raw JSON retained, not inferred from top loader. D18 has18/18 blocks,86434/34,Data dtor4/4,top dtor5/5. Data ctor's extra unused blocks and top ctor's unused blocks are compiler exception cleanup; all nonthrow edges and executable header lines ran. E84 is fully inlined into Data ctor, all five stores at header28–32 execute. |
| Top .inc | 15/15 lines,17/17 edges ALL-ARMS. |
| Getter header | Each1/1 lines,0 branches; raw function counters633/1/91 for listening/search/gravity. |

Reproduce (wait for a game slot first):

```sh
BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py SECS=300 \
  tools/cov/force.sh tools/cov/specs/sjg2_load_arms.py <out> \
  42b935e4,02_sand_sandfalls_dark --con tools/drivers/sjg2.con --fast --frames 1200
```

Final loader run exitsnormally at1200 frames, caught/reach-stub0. Earlier6500-frame baseline
with `ingspacejumpguardian_sandfalls_forced.con` also exitsnormally/caught0 and is only a
guide for continuation, not certification of the remaining runtime bodies. It reaches
hp160,notDead: do not assume that driver's shooting alone covers the death state.

### Static review / checkpoint gates

asmcheck's getter/loader parsing limitation reports MISSING despite literal markers and
real gcov bodies; all four are manually audited above. NOPOS/BIT A34 findings are the
seven independently decoded flag masks above, with member assertions. Calls63D0=data copy,
CPathFindSearch=member initializer,Lock=locked-token implementation; internal_dereference
is temporary-string cleanup. Constructor BRANCH mismatch is MW optional/token lifetime
checks versus native optional ownership. Header XSTORE findings mistake property-hash
comments for offsets: actual member slots are in the complete data tables above. E84
calls are member initialization, ADC/7E8 calls are implicit reverse member destruction;
E48 is the empty stage dtor; Free belongs to caller-owned deleting-wrapper storage.

retail_diff incorrectly associates170 with CollisionDamage; the actual loader is1181
and was fully audited. Its three leads are aliases:864 is invoked through the .inc,
4A0 is optional-model copy,constant360 is the inline top-loader ctor default in the header.
No unresolved static finding. No production probes or shared-engine/tool changes.

Host and coverage ninja links rc0,0 FAILED; manifest rc0; matching configure+ninja rc0,
All77.98% fuzzy/54.39% matched,Game94.14%/63.29%; dtk shasum **87 files OK**. Three upstream
files are marker-only, non-PC strip-diffs CLEAN. Reviewe32d198ab:eleven MARK,no other findings.

### Continuation:18 bodies still unmarked

`ED4 UpdateBlob`, `1198 FindJumpPoints`, `1660 FilterJumpPoints`, `1BFC CameraShake`,
`216C UpdateLight`, `2498 SetCollisionVulnerability`, `2534 CollisionDamage`,
`2874 UpdateCollisionVulnerabilities`, `2ED0 ComputeJumpVelocity`, `34CC SelectJumpTarget`,
`36B8 SteerToDest`, `3888 PathFind`, `3DA8 Jump`, `46D0 FollowIntroPattern`, `4864 Dead`,
`500C GetAimPosition`, `5274 DoUserAnimEvent`, `5760 AcceptScriptMsg`.
All full listings are saved; ED4/216C/2498/2874/2ED0 were read against the current source
without finding another mismatch, but no live-arm certification/marker is claimed yet.

## sjg3 runtime checkpoint (2026-10-24)

Base `293c09dab`, nine new markers, **51 → 60/69**. No production behaviour changes.
Full `asmcheck.py annotate ... --all-ins` listings for all eighteen assigned bodies:
`/tmp/opencode/reports/sjg3-evidence/asm-*.txt`. The nine marked bodies were audited
instruction-by-instruction against the existing source, including previous TARGET_PC fixes.

### Retail block/arm audit and execution

All setups below are **forced (`tools/cov/specs/sjg3_runtime_arms.py`, inferior calls at
first active Think)**. Real bodies and callees run; no callee result is replaced or skipped.
The input members are restored after each call. Collision manager/light already exist.

| body | retail correspondence / executed arms | final gcov |
|---|---|---|
| ED4 UpdateBlob | EF0–F0C lookup/null; F10–F30 byte A34 bit04 selects 1/0 emission; F38–F3C positive intensity gates transform; F40–FF4 builds LookAt at actor position toward world up; FF8–1000 emission setter. Real Dead activation creates blob; active0/1 and invalid blob id execute all live arms. | 10/10 lines, 5/6 edges; dead constant arm below |
| 216C UpdateLight | 2184–21B0 base byte420 bit40 = alive: saturate intensity at1; 21B4–21D0 light lookup/cast; 21D4–2264 black→data colour lerp, head locator, point light, attenuation(1/data,0,0), SetLight. Alive0/1, missing light, intensity0/2 with dt.25 cover both saturation outcomes. | 10/10, 4/4 ALL-ARMS |
| 2498 SetCollisionVulnerability | 24A8/2500–2510 unsigned count loop; 24C8–24F4 description id lookup/cast/null; 24F8–24FC vulnerability copy. Real reflect vulnerability, ordinary seven actors, one invalid description id and count0. | 7/7, 4/4 ALL-ARMS |
| 2874 UpdateCollisionVulnerabilities | 2898–28B0 health virtual slot3C and full health copy2998; 28B4–2974 count loop; 28BC–28E8 description lookup/null; 28EC–2960 qualified base vulnerability, virtual HealthInfo slot38 and full health assignment. Same ordinary/invalid-id/count0 inputs. | 9/9, 4/4 ALL-ARMS |
| 2ED0 ComputeJumpVelocity | 2F10–2F4C jumping bit80/invalid-id/absent actor guards; 2F50–2FA0 deltas, start/target heights, gravity virtual slot114; 2FA8–2FBC higher-height selection; 2FC0–3028 vertical sqrt, flight time, XY/time, SetVelocityWR; 302C–303C jumping bit80 and duration stores. True/false jumping, FFFF/FFFE/player target; explicit actor height playerZ±5 covers both apex arms with apex offset10. | 20/20, 6/6 ALL-ARMS |
| 36B8 SteerToDest | 36D4–36F8 message dispatch; 36FC–3740 Ready1, walk/run speed ratio, full-speed steering; 3748–3760 missing target; 3764–37E0 safe delta and positive forward dot; 37E4–3840 normalized facing cmd; 3848–3858 Over3; 3860–3864 NotReady0. Msg0/1/2/3, invalid/player target and delta0/±forward. | 22/22, 10/10 ALL-ARMS |
| 3888 PathFind | 38B0–38CC dispatch; 38D0–38F0 full-speed range1; 38F4–3914 path bit40 guards navigation; 3918–3924 heard bit20 clear; 392C–3940 Update path bit40; 3944–3954 search index versus count−1 (tail runs for every msg); 3958–39A8 negative forward dot and safe magnitude; 39AC–3A14 clear and normalized facing cmd. Msg0/1/2/3 with path0/1, over/not-over, forward/backward and tiny backward input. | 19/19, 15/15 ALL-ARMS |
| 4864 Dead | 489C qualified base call; 48A0–48B4 dispatch; 48B8–4948 default speed, real PART token/blob spawn, timer2; 4950–496C UpdateBlob and body Death-state guard; 4970–49A4 boss-id clear; 49A8–49F8 scaleZ=max(0,Z−dt/.75); 49FC–4A34 <=0 shrinks timer and clears render02/blob04; 4A38–4A58 timer<=0 DeathDelete. Msg0/1/2, real body Locomotion/Death states, matching/nonmatching boss, positive/zero scale and timer2/0. Deletion is last, not stubbed. | 24/24, 13/13 ALL-ARMS |
| 500C GetAimPosition | 5044–5094 zero prediction unless dt>0; 5098–50A0 collarFF guard; 50A4–510C locator transform, actor transform, component scale and predicted translation; 5114–5130 qualified base fallback; 5134–513C result. dt0/.25 with real collar/FF. | 12/12, 4/4 ALL-ARMS |

ED4's only untaken edge is source1182, the Up-versus-Right choice. Retail F54–FA8
compares **the global world-up Z**, not actor up: `AbsF(float(Up().Z < .95)) != 0`.
World-up is initialized `(0,0,1)` at `src/Kyoto/Math/CVector3f.cpp:16`; its public accessor
returns const reference, and the source-tree symbol scan finds no other writer. Therefore
the Up arm cannot occur in the shipped program; Right is the executed retail arm. No
global vector mutation was used to manufacture coverage.

### Reproduction and limitations

```sh
tools/games_running.sh --wait
BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py SECS=300 \
  tools/cov/force.sh tools/cov/specs/sjg3_runtime_arms.py <out> \
  42b935e4,02_sand_sandfalls_dark --con tools/drivers/sjg3.con --fast --frames 1200
python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module IngSpaceJumpGuardian
```

Final run:55 successful calls, no failed assignments/calls/Python errors, normal exit,
caught/reach-stub0. Raw profiles/gcno and logs are retained in the evidence directory.
The death fixtures are explicit state/body-member setup, not a claim the shooting driver
naturally reaches death. Input writes restore members, but real body side effects remain;
fixtures are ordered so queued deletion is last. No shared-engine/tool edits or probes.

FollowIntroPattern is **not marked**:21/22 lines,8/9 edges; the <=0 dot arm/source560
still did not run despite attempted position setups. Do not equate successful inferior
calls with covering that arm. Remaining9:1198 FindJumpPoints,1660 FilterJumpPoints,
1BFC CameraShake,2534 CollisionDamage,34CC SelectJumpTarget,3DA8 Jump,
46D0 FollowIntroPattern,5274 DoUserAnimEvent,5760 AcceptScriptMsg.

### Review findings and gates

asmcheck MISSING for ED4/216C/2498 is its existing definition-parser failure (literal
markers and real compiled/gcov bodies exist). NOPOS/BIT for A34 are decoded above:
80 jumping,40 hasPath,20 heard,04 blob,02 render. CALL517C is the native copy of
PredictMotion's CMotionState;2998 is CHealthInfo copy;2280 is CLight copy. These helpers'
full instruction listings were inspected. CALL1024/ED4 in Dead are the literal
SpawnBlobEffect/UpdateBlob calls, missed by the same parser. retail_diff mislabels
216C/1024/ED4 as CollisionDamage; its five call leads are exactly these copy/parser cases.
No unresolved correctness finding or new behavioural fix.

Host/coverage links rc0,0 FAILED; gen_files_cmake --check rc0; upstream configure+ninja
rc0, All77.98% fuzzy/54.39% matched,Game94.14%/63.29%,dtk shasum87 files OK.
Only upstream-file diff is nine marker comments, so non-PC strip-diff CLEAN.
`tools/review_decomp_diff.sh 293c09dab`:9 MARK, no other findings; all nine accounted for above.
Unchanged families and whole-module smoke skipped per scope.
