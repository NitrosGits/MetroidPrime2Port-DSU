# DarkCommando REL (module 3): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CDarkCommando.cpp` (Dark Pirate
Commando, its `CShadowDecoy`, `CDarkCommandoGrenade` EMP grenade and `CDarkCommandoChargeBeam`);
the loader defaults are `include/MetroidPrime/ScriptLoader/SLdrDarkCommando.hpp`. The port's fixes
are `TARGET_PC` blocks in those two files. Retail asm:
`build/G2ME01/DarkCommando/asm/MetroidPrime/Enemies/CDarkCommando.s` (after `python3 configure.py && $T/ninja`).
Per-function table: `DarkCommando_functions.tsv` (B = module body, H = weak copy/template or unnamed
static, T = REL runtime).

## Behaviour fixes (dc1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_3_19C LoadDarkCommando (inlined SLdrDarkCommando ctor, struct at r1+0x400) | 0x218 `stw 0x438` editor 0x38 = 3; 0x234 `stb 0x66c` isEchoEmitter 1; 0x244 `stw 0x6e4` visorFlags 0xF; 0x254 `stw 0x694` ambient CColor(1,1,1,1) | 0 / false / 0 / 0 |
| fn_3_14B8 SLdrDarkCommandoEMPData ctor | 0x14D4 `li r0,-1`; 0x150C/0x151C `stw r0,0x40/0x44(r31)`: sound_GrenadeBounce/Explode -1 | 0 |
| fn_3_8BEC CDarkCommandoGrenade::Think | 0x8C24 `extrwi. r0,r0,1,24; beq 0x8D5C`: the EMP interference and the delete run only once exploded | only `mEMPTime += dt` gated |
| fn_3_3104 BuildActionChoices | 0x33F4 `clrlwi. r0,r3,24; beq 0x34A4`: returns when the RayCollideWorld line of sight is blocked (false) | returned when clear (inverted) |

Checked equivalent (no fix): SelectMeleeVariant, PreRender, Dead, CShadowDecoy::Think (redundant
null check), CDarkCommandoChargeBeam::Think (`5 - t/60` form), ThinkRagDoll (radii copy loop),
every state body (switch reordering only: Lurk, Taunt, FaceTarget, MeleeAttack, FireEMP,
PostFireChargeBeam, ShadowDash, WarpIn/WarpOut), KnockBack, SLdrDarkCommandoData (fn_3_B14),
ChargeBeamData (fn_3_10E8) and ShadowDashData ctor defaults; BuildActionChoices' `Magnitude() > 15`
AsNormalized (result discarded, dead code); SolveGrenadeLaunch (retail keeps the best angle/speed in
f28/f27, seeded with 0.349 and grenadeMinLaunchSpeed, and stores after the loop; the one caller
passes those same seeds); LaunchGrenade (retail computes the launch locator before the target
lookup, no side effects); FireChargeBeamShot (retail inlines TCachedToken::IsLoaded on
ProjectileInfo()->Token()); SelectDashTarget (target/self positions hoisted; same compares,
`>=`/`<=` via `cror`, NaN falls to the else arm in both); ApplyMeleeDamage, UpdateAdditiveAim, the
CDarkCommando ctor (inline CSfxHandle store). `upstream_hollow.py DarkCommando`: none.
CreateShadowDecoyEffect's eager token load is restored by dc3 below.

## Behaviour fixes (dc2, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_3_1650 JoinTeam | 0x1674-0x1680 `lhz r5,0xc90; cmplw r5,kInvalidUniqueId; bne 0x16E4`: a set team id skips the whole join | re-joined on every SelectDashTarget; `CScriptTeamAiMgr::JoinTeam` then reset the member's role |

Checked equivalent (dc2): ShouldMeleeAttack, AcceptScriptMsg (OffGround `lbz 0x594; extrwi 1,26`
= frozen, skip), ApplyMeleeDamage (wrist 0xc79 / blade 0xc78 min/max, 3*scale.y, `<= 0` timer),
LeaveTeam, ReserveHint, CShadowDecoy::Touch, ShouldGetUp (states 0xe Hurled / 2 LieOnGround),
UpdateAdditiveAim (null target: no AdditiveIdle either), WarpIn, Lurk, Taunt, FaceTarget (`<=`
0.1745 via cror, NaN skips), InEMPRange/InChargeBeamRange (same shape, 0x814/0x818 vs 0x85c/0x860).

## Spawns

Temple Grounds dark (world 3bfa3eff) 06_temple_sacredpath_dark (area 55): two Dark Commandos, u051a
(e0037014e, near (-6.4,282.4,-38.7)) and u041c (e00370056, (5.6,244.6,-30.4), inactive until
`send u041c Activate`); 650 hp each. Team: 3bfa3eff 11_temple_grandpuzzle layer 8 has three commandos
(u065c/u065a/u0659) and their CScriptTeamAiMgr u065b (`layer 8` activates them). Others:
docs/modules.tsv row DarkCommando.

## Coverage (dc1)

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON` (`-DMP_SDK_HEADERS_ONLY=OFF`); runs (not `--fast`:
the driver is slower than the game and runs out of frames):

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/darkcommando.py <out> 3bfa3eff,06_temple_sacredpath_dark \
      --driver 'python3 tools/drivers/darkcommando_states.py {port} <out>/states.log' --frames 24000
    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/darkcommando.py <out> 3bfa3eff,06_temple_sacredpath_dark \
      --con tools/cov/specs/darkcommando_fight.con --frames 4200 --env MP2_GODMODE=1
    python3 tools/cov/gcov_report.py build-port-cov <out1>/gcov <out2>/gcov --module DarkCommando

`darkcommando_states.py` forces every FSM state by name (`SelectAttackTarget`, `SelectAction`,
`SelectDashTarget`, `Set*FaceVect`, `ActivateCloak`/`DeactivateCloak` are code funcs inside
states, not states: `fsm` cannot reach them), damages, replays the attacks and kills both;
`darkcommando_fight.con` is the natural fight (charge beam, EMP grenade, shadow dash, cloak). Merged:
88 bodies, 64 with branches, 7 with every arm taken.

Marked by dc1 (24; every arm taken or branch-free with every line run, objdiff 100% or
label/argument-only differences): ReleaseHint, GetHint, SetCloakTarget, SelectAttackTarget, Start,
ShouldWarpOut, ShouldFireChargeBeam, ShouldFireEMP, ShouldTaunt, StateOver, SetupStateMachine,
ProjectileInfo, GetAimPosition, AddToRenderer, PreRenderAllViewports, PreThink, ~CDarkCommando,
~CDarkCommandoChargeBeam, CDarkCommandoChargeBeam::AcceptScriptMsg, ~CDarkCommandoGrenade (upstream
adds a null check), CDarkCommandoGrenade ctor, ~CShadowDecoy, CShadowDecoy::GetOrbitPosition/GetAimPosition.

Unmarked (arms not run): the fixed BuildActionChoices (11/16 arms) and CDarkCommandoGrenade::Think
(10/12; the `eMPDuration > 0` false arm and the not-yet-expired arm need a forced duration);
ApplyMeleeDamage and SelectMeleeVariant (melee never landed: 0/10, 1/12), ShouldMeleeAttack,
KnockBack (12/24), every state body (exit or timer arms missing), Think, AcceptScriptMsg,
DoUserAnimEvent, FireChargeBeamShot, LaunchGrenade, SolveGrenadeLaunch, GetGrenadeTargetPosition,
InEMPRange/InChargeBeamRange, the CShadowDecoy bodies, the charge-beam ctor/Think/collision,
ThinkRagDoll, JoinTeam/LeaveTeam, UpdateSfx, LoadDarkCommando (one arm), the CDarkCommando ctor.
Next: a melee driver (player within melee range, `fsm MeleeAttack` until the blade hits), force the
grenade's EMP duration and the team (`CTeamAiMgr`) arms.

## Coverage (dc2)

Drivers/specs: `tools/drivers/darkcommando_melee.py <port> [log] [layer|-] [leave]` (activates every
commando, places one at several distances, forces `fsm MeleeAttack`, then FireEMP at 12-30 units;
`leave` ends with 3x `send Deactivate`); `tools/cov/specs/darkcommando_force.py` (gdb forces,
self-restoring); `darkcommando_emp_force.py` (grenade EMP duration only). Runs merged (force first):

    MP2_GODMODE=1 BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darkcommando_force.py <f> 3bfa3eff,11_temple_grandpuzzle \
      --driver 'python3 tools/drivers/darkcommando_melee.py {port} <f>/melee.log 8 leave' --frames 9000
    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/darkcommando.py <t> 3bfa3eff,11_temple_grandpuzzle \
      --driver 'python3 tools/drivers/darkcommando_melee.py {port} <t>/melee.log 8' --frames 9000 --env MP2_GODMODE=1
    (+ the dc1 states run, and the melee driver in 06_temple_sacredpath_dark, 9000 frames, godmode)

Merged: 88 bodies, 64 with branches, 11 with every arm taken. Marked by dc2 (14):

| fn | arms | note |
|---|---|---|
| fn_3_8BEC CDarkCommandoGrenade::Think | 12/12 | duration arms forced (darkcommando_force: eMPDuration) |
| fn_3_9184 CShadowDecoy::Touch | 2/2 | natural |
| fn_3_15A8 LeaveTeam | 6/6 | not-a-member / not-a-team forced (darkcommando_force: LeaveTeam calls 2/3) |
| fn_3_2D24 UpdateAdditiveAim, fn_3_6030 ShouldGetUp | all | natural |
| fn_3_1650 JoinTeam | 5/6 | cast-null dead: the id comes from GetAssociatedTeamId in the same call (after the fix) |
| fn_3_1F34 ReserveHint | 1/2 | cast-null dead: the only caller (SelectDashTarget) passes hint ids it collected from the AiWaypoint list in the same call |
| fn_3_5DA8 WarpIn, fn_3_5A2C Lurk, fn_3_58D8 Taunt, fn_3_484C FaceTarget | all but 1 | the `else if (msg == kStateMsg_Deactivate)` false arm is dead (EStateMsg is 0..2); Lurk/Taunt knocked-back and FaceTarget zero vector forced |
| fn_3_3690 ApplyMeleeDamage | 9/10 | `!mMeleeDamageApplied` false arm dead: 0xcb4 bit 0x01 is only written 0 (0x5664 MeleeAttack Activate, 0x7388 ctor); not-attacking / zero blade forced |
| fn_3_3054 InEMPRange, fn_3_2FA4 InChargeBeamRange | 9/10 | target-null dead: the only caller, BuildActionChoices, returns on a null target first; min range forced |

Still unmarked: ShouldMeleeAttack, SelectMeleeVariant, MeleeAttack, FireChargeBeam (target-null
arms reachable from the FSM but not run), BuildActionChoices (1238-1245 arms this run),
DeactivateCloak (1/4), UpdateCloak (2/4 dark-visor arm), FireEMP, ShadowDash, PostFireChargeBeam,
WarpOut and GetUp (0%: never entered), KnockBack, ThinkRagDoll, the charge beam and decoy bodies.

## dc3 audit and fixes (2026-10-24)

Base `58da92ffc`, branch `decomp/dc3`. **32 additional markers, 70/100 behaviour bodies.**
The dc3 brief explicitly restricts new runtime forcing to changed behaviour or questions not settled
by asm. Accordingly the new markers record complete asm audits, not new all-arm runtime coverage:
the unchanged bodies retain the dc1/dc2 runtime limitations above. Null-target arms below are
statically established, not asserted dead. The focused run tests the two changed bodies.

Retail source: `../MetroidPrime2Port/build/G2ME01/DarkCommando/asm/auto_00_00000000_text.s`
and its adjacent rodata/data; `tools/asmcheck.py annotate DarkCommando fn_3_<off> --all-ins`.
The matching build's `build/G2ME01/report.json` supplies the supplementary match percentages.

### Changes

| body | defining asm | fix |
|---|---|---|
| fn_3_3C44 ActivateCloak | `0x3C74 fcmpo cr0,f0,f1; 0x3C78 bgt 0x3C88; 0x3C80 fcmpo cr0,f0,f1; 0x3C84 ble 0x3F34`; operands are alpha/target alpha and zero | The failed alpha gate exits the **whole** body, including decoy deletion/allocation and orbit redirection. Upstream only gated the fade/sound block. A TARGET_PC early return preserves `!(alpha > 0 || target > 0)`, including unordered comparisons; the existing hurled gate remains first. |
| fn_3_9754 CreateShadowDecoyEffect | `0x97B4 bl CToken::CToken; 0x97BC bl CToken::GetObj; 0x97C0 lwz r0,4(r3); 0x97CC stw r0,0x28(r1)` precede `0x97EC bl __nw` | Use `TLockedToken<CGenDescription>` under TARGET_PC: copy/load/cache before allocation. Previously the plain token deferred loading to CElementGen's initializer. Valid and invalid asset paths stay the same; resource acquisition order now matches retail. |

### Audited bodies and branches

Every row is marked. `F` means the changed path was forced by `darkcommando_dc3.py`; `A` means
asm audit with no new runtime-coverage claim. Existing dc1/dc2 runs remain applicable to unchanged
code. No `<=`/`>=` was substituted for a plain PPC branch, or vice versa.

| fn_3_ | body | evidence / branch result |
|---|---|---|
| 2058 | SolveGrenadeLaunch | A: 0x2194/0x21A0 selects upward start/negative step; 0x2204/0x2208 denominator `> FLT_EPSILON`; 0x2210-0x2224 inclusive speed interval; 0x2238-0x2250 nearest-error fallback; ten candidates. Retail seeds outputs internally; the only caller LaunchGrenade passes exactly those seeds, so failed-solve behaviour agrees for every live call. |
| 230C | GetGrenadeTargetPosition | A: 100% matching; 0x2380/0x238C player/unmorphed gate, 0x23E8/0x23EC distance `> 6`, pull aim 3 units toward owner. |
| 2460 | LaunchGrenade | A: 0x24DC null skips launch; 0x2588 non-normalizable horizontal uses forward; 0x25D4 plain `ble` caps only angles `> pi/3`; allocation/add tail. Retail computes the locator before the null check (no externally visible effect). |
| 28DC | FireChargeBeamShot | A: 0x293C null exits; 0x29B4 player prediction; 0x2A9C plain `ble` gates angle `> pi/6`; 0x2B58-0x2BA0 loaded token gate; 0x2BC8 projectile budget; allocate/add. Retail lazily caches the already-locked weapon token; host IsLoaded tests the same availability and the projectile obtains the object. |
| 3104 | BuildActionChoices | A: 0x3180-0x3188 alpha `<= 0` adds lurk, always taunt; 0x3224 null exits **after** those choices; 0x3238/0x3268 team permission; dc1's LOS fix at 0x33F8; 0x340C/0x3460 EMP/beam ranges. 0x3308-0x3318 computes/discards a normalized vector when magnitude `> 15`: omitted dead calculation, no field store or call with observable effect. |
| 34B8 | SelectMeleeVariant | A: 0x3504 null returns -1; 0x3584-0x35C4 three inclusive max-angle tests; 0x35E0-0x365C inclusive weighted selection. Retail unrolls both three-element loops (nine cror compares); C++ has three comparison sites, with identical NaN exclusion. |
| 39AC | UpdateCloak | A: 0x39CC fade flag; 0x39E4/0x3A08 `<`/`>` interpolation, 0x3A40 abs error `< 1e-5`; 0x3A78 positive alpha OR dark visor adds target material; 0x3AC4-0x3AF0 stale decoy reset. |
| 3B98 | DeactivateCloak | A: 100% matching, 0x3BB8-0x3BC8 either alpha `< 1`; fade, visor/shadow and sound all inside that gate. |
| 3C44 | ActivateCloak | F: whole-body hurled/alpha gate above. A: 0x3D14 existing decoy deletion, 0x3EC8 allocation guard, 0x3EF4 null/non-player skips **only orbit redirection**, 0x3F04 orbit id and 0x3F18 dark visor checks. |
| 3F48 | SetTargetFaceVect | A: 0x3F8C null leaves forward default; 0x3FC8 unsafe XY does likewise; 0x3FF8/0x400C bone-tracking activation/target assignment still run in both cases. |
| 4024 | SetDashFaceVect | A: 0x4058 missing hint and 0x4094 unsafe XY keep forward default, else normalized XY. |
| 40D0 | SelectAction | A: 100% matching; sum weights, 0x41E8-0x41F0 inclusive selection, subtract failed weight, default lurk; 0x421C clears knocked-back even when no choice selected. |
| 4310 | SelectDashTarget | A: 0x4390 null jumps to JoinTeam tail at 0x45F4, never an early return; 0x43F4-0x4460 active/type/in-use/area/previous/safe-zone filters; 0x449C-0x456C inclusive distance tests and preferred/fallback list resets; 0x4590 capacity 128; 0x45C4 empty list skips reserve; 0x45FC always joins via dc2's corrected helper. |
| 46E4 | GetUp | A: 0x4720 ready + decloak + clear vertical movement/hurled; 0x475C command eligibility; exit 0x47CC not-ready + restore vertical movement. Previously unobserved runtime state, not proven dead. |
| 4984 | ShadowDash | A: 0x49DC-0x4B04 ready/dash flags, missing hint sets over but still changes filter; 0x4B20 command eligibility; 0x4BBC/0x4BC8 loop-attack + moving; 0x4BDC missing hint exits body state; 0x4C24 distance `> speed*dt`, else snap/exit; deactivate aborts loop and restores all six material bits. |
| 4ED8 | PostFireChargeBeam | A: 0x4F2C knocked-back OR 0x4F4C not `(time < idle)` sets over (NaN sets over); 0x4F68 null skips facing; 0x4FC8-0x4FD0 inclusive angle gate; ready/not-ready lifecycle. |
| 5074 | FireChargeBeam | A: 0x50C4/0x50F0 team/no-team start; 0x512C command eligibility; 0x5148 null -> 0x521C over; valid aim command severity 2; exit clears firing and ends projectile attack. |
| 52D8 | FireEMP | A: team/no-team start; 0x5394-0x539C time `>= prefire`; 0x53D0 null after warmup sets over, while 0x54D4 null before warmup exits without command; 0x54B8 knocked-back sets over; 0x5534-0x553C angle `>= pi/6`; exit resets/ends projectile attack. |
| 5614 | MeleeAttack | A: 0x5674 variant -1 sets over; 0x5688/0x56B8 team permission; 0x5700 command eligibility, else 0x57E0 null/non-physics skips target vector AND damage; exit clears attacking/ends melee attack. |
| 5B6C | WarpOut | A: 0x5BA8 clears flag on entry; 0x5BBC/0x5BCC pending generation/deletion; 0x5BF8 generation begins fade/removes target; 0x5C4C-0x5C60 alpha `>= 1` and unfrozen issue generation. Previously unobserved runtime state, not proven dead. |
| 5F5C | Dead | A: entry 0x5FA4 gates alpha `< 1` and enables visibility; update 0x6000-0x6008 gates base Dead on alpha `>= 1`. 0x5FDC's 255 is the inlined CColor float-to-byte conversion. |
| 6070 | ShouldMeleeAttack | A: 100% matching; 0x60A8 null -> false at 0x6154; team permission then inclusive squared range (0x6134-0x6148). |
| 8240 | ChargeBeam::ResolveCollisionWithActor | A: 100% matching; player/mold/token gates, reserve billboard/player ids, safe-zone check, billboard flags run-indefinitely/finishing; 0x8444 always calls base collision (including null-player/billboard paths). |
| 8598 | ChargeBeam::Think | A: 0x85D4 invalid billboard -> base Think; missing effect -> delete; 0x8640/0x8648/0x865C timer `>= 5`, missing player or safe zone accelerate effect expiry; otherwise damage scaled by dt; remaining `5 - emitterTime/60 > 0.3` uses a plain GT test at 0x86B4/0x86B8. |
| 87A4 | ChargeBeam ctor | A: 100% matching including optional mold-token copy/lock, damage copy, invalid ids, zero timer and REL token. |
| 91F0 | Decoy::GetDamageVulnerability | A: 100% matching, returns own vulnerability. |
| 91F8 | Decoy::Render | A: 100% matching, base Render. |
| 9218 | Decoy::AddToRenderer | A: 100% matching; base call, effect-present gate, exclude dark/echo visors, submit particles. |
| 92A4 | Decoy::Think | A: 0x92D8/0x92F0/0x9308 missing effect, health `<= 0`, or deletable -> deletion/orbit reset/sound removal; else update. The C++ final effect recheck's false arm is dead: the same effect already passed the initial null check and no writer intervenes. |
| 93A4 | Decoy::AcceptScriptMsg | A: 100% matching; base first; Delete removes/clears a valid emitter. |
| 9414 | Decoy ctor | A + F through creation: copied bounds/vulnerability/health/owner/orbit fields; 0x95E0 effect guard for emission/orientation/translation/scale/sound; 0x9694-0x96C4 disable shadow, all visor flags on, scan/dark off. Sound argument is a value copy in C++ versus a reference in retail, with unchanged contents. |
| 9754 | CreateShadowDecoyEffect | F: invalid asset -> null; valid asset -> eagerly locked token before CElementGen construction; normal orientation/flag one. Allocation failure remains statically audited, not forced. |

### Focused runtime evidence

    MP2_FAST_LOAD=1 tools/cov/force.sh tools/cov/specs/darkcommando_dc3.py <out> \
      3bfa3eff,06_temple_sacredpath_dark --con tools/cov/specs/darkcommando_fight.con \
      --frames 6000 --fast

The spec saves/restores artificial alpha/flag/asset inputs and checks actual decoy constructor
counts and id changes at return. **9/9 cases PASS**: positive alpha, both zero, both negative, both
NaN, positive target only, hurled, invalid effect, NaN alpha/positive target, positive alpha/NaN
target. Skipped cases made zero decoys; passing cases made one; the invalid effect made none of the
particle generators; all four valid descriptions held their lock before the generator constructor.
Area 55, 6000-frame limit, normal exit, `[port] caught` / `reach-stub` **0**. No source probes.

Runtime gaps retained: unchanged null-target arms, GetUp/WarpOut entry, the previous team/attack,
visor/safe-zone/decoy lifecycle coverage gaps, and allocation-failure arms were not newly forced.
These are **not** evidence of dead code; their behaviour is settled by the asm table. Of the
32 new markers only the changed cloak gate/resource load receive new targeted runtime evidence.

### Check findings and gates

- Host build: `ninja -C build-port-link metroid_prime2_port`, 0 FAILED. Manifest check rc 0.
- TARGET_PC strip-diff against base: identical non-PC bytes after removing marker comments.
  Matching build: `87 files OK`; final progress unchanged, All 77.98% fuzzy / 54.39% matched;
  Game Code 94.14% fuzzy / 63.29% matched.
- `asmcheck` correctness tags on all 32 additions: **0 findings**. All tags: 44 findings plus
  NOPOS, manually resolved below (the shared header has guessed names without retail bit positions).
- BIT/NOPOS: `0xcb4`: melee 0x80, EMP 0x40, beam 0x20, warp-in 0x10, warp-out 0x08,
  fading 0x04, shadow-dash 0x02, damage-applied 0x01; `0xcb5`: dash-moving 0x80,
  warp-request 0x40, knocked-back 0x20, hurled 0x10. All relevant tests/stores in the table match
  the named C++ fields. GetUp's base vertical-movement field is `0x34c & 0x40`.
- CALL/CALLS: ctor initializer lists supply CModelDataNull (27C0), vulnerability copy (7608),
  impact-effect None (8970)/destructor (800C), optional locked-token copy/Lock and REL-token ctor;
  temporary string/EntityInfo destructors (2830) are automatic C++ lifetime calls. Mutable vs const
  TCastToPtr/GetObjectById wrappers share the same casts/lookups. FireChargeBeamShot's ProjectileInfo
  calls are retail virtual slot `+0xdc` (0x29F0, 0x2B4C, 0x2C84, 0x2C9C); IsLoaded vs lazy caching
  is described above. BuildActionChoices' unused vector math is described above.
- BRANCH: SelectAction's vector loops are unrolled in retail; ResolveCollisionWithActor's optional
  token/string temporaries add destruction/placement-new guards; Decoy AcceptScriptMsg/Think/Render
  and ChargeBeam Think counts are inflated by the checker's helper folding of same-named methods
  across classes (e.g. base Render is not another copy of CDarkCommando::Render).
- `retail_diff`: only BuildActionChoices' unused Magnitude/AsNormalized/15, SelectMeleeVariant's
  unrolled nine cror comparisons, and Dead's inlined 255 alpha conversion; no unresolved difference.
- `review_decomp_diff.sh 58da92ffc` after commit: MARK 32, with the body/arm evidence above;
  no code removals or shared-code edits. The script's ASM pass only selects Port*.cpp; the explicit
  full/correctness asmcheck runs above cover this upstream-named file.

## dc4 runtime closure (2026-10-24)

Base `39af4e5ef1cc103b37ac5136939952bdd8bbad8e`, branch `decomp/dc4`. **26 additional markers,
96/100 B rows marked; four implicit copy constructors remain unmarked.** No new gameplay
divergence was demonstrated: all three C++/header diffs contain marker comments only. The
following evidence strengthens the earlier asm-only markers as well as closing the new bodies.
It does not turn dc3's entire runtime-gap list into an all-arms claim.

### New markers: per-body evidence

`F` below means forced with the committed GDB spec plus console driver. Counts are compiler
coverage, excluding `throw:true` edges. Host EH cleanup descendants need the further qualification
below. These are executions of the real module bodies; dependency return values are forced only
where explicitly stated. Retail source is the `auto_00_00000000_text.s` path cited in dc3.

| fn_3_ | body | evidence / live arms |
|---|---|---|
| 8 | GetGravityConstant | Retail 0x8/0xC loads rodata_0 = 50; host inline getter ran 611 times in the final loader run (1/1 blocks). |
| 16C | SetFuncPtrs | Branch-free registration; matching 100%; inlined into RELMain, whose 2/2 blocks ran once before the loader was called. |
| 19C | LoadDarkCommando | Host CPP 10/10 branches; valid model and `spawn DRKC 1 inactive` missing-model refusal. F(loader spec): top-level actor-parameters hash changed to 0xfeedbeef, same encoded size, bytes restored at return; default actor parameters retained for that one actor. Final `.inc` coverage hits every case, default, loop entry/exit; consumed 4963/4963 bytes. Retail allocation caveat below. |
| 820 | ~SLdrDarkCommando | Host stack teardown ran 3 times, 5/5 blocks; retail wrapper null/deleting arms have no references (see lifetime proof). |
| 89C | LoadTypedefDarkCommandoData | 28/28 blocks, every property and unknown skip; F(loader spec) replaces lurkChance hash and restores its value; consumed 2934/2934 bytes. |
| A80 | ~SLdrDarkCommandoData | Host 7/7 blocks; stack/member teardown, null/deleting wrapper proof below. |
| B14 | SLdrDarkCommandoData default ctor | Branch-free retail initializer sequence at 0xB2C–0xBBC; normal host initialization ran; remaining host blocks are EH cleanup. |
| BC8 | LoadTypedefDarkCommandoShadowDashData | 28/28 blocks; F(loader spec) replaces unknown_0xa0d037ee hash, then restores scalar; consumed 2293/2293 bytes. |
| DB4 | ~SLdrDarkCommandoShadowDashData | Host 6/6 blocks; member teardown, wrapper proof below. |
| E3C | SLdrDarkCommandoShadowDashData default ctor | Corrected the TSV's mistaken top-level-loader identity. Retail 0xE54 writes invalid asset at +0x10; 0xE5C/64/6C construct three audio records at +0x14/+0x2c/+0x44; 0xE74/7C construct vulnerabilities at +0x5c/+0x1b8. Stores 0xEA0/AC/B0/B4 initialize 25,40,25,20 (rodata_7C/8C/84). Branch-free normal initialization ran. The matching report retains the old, wrong symbol name; it is not identity evidence here. |
| ECC | LoadTypedefDarkCommandoChargeBeamData | 24/24 blocks; F(loader spec) replaces postFireIdleTime hash, then restores scalar; consumed 213/213 bytes. |
| 1078 | ~SLdrDarkCommandoChargeBeamData | Host 4/4 blocks; member teardown, wrapper proof below. |
| 10E8 | SLdrDarkCommandoChargeBeamData default ctor | Branch-free retail defaults including two damage records and audio; normal host path ran, EH-only blocks excluded. |
| 1198 | LoadTypedefDarkCommandoEMPData | 38/38 blocks; F(loader spec) replaces preFireIdleTime hash, then restores scalar; consumed 190/190 bytes. |
| 1460 | ~SLdrDarkCommandoEMPData | Empty host destructor is elided; enclosing data/grenade teardown ran. Retail non-deleting member path is branch-free after its ABI guards, proven below; matching 100%. |
| 14B8 | SLdrDarkCommandoEMPData default ctor | 2/2 host blocks; branch-free retail defaults, including dc1's -1 sound defaults. |
| 16FC | UpdateSfx | 6/6 branches. F(lifecycle): zero handle; nonzero playing; queued; neither. Real queries run, their returns are forced; all four handle assertions PASS. |
| 17A0 | ThinkRagDoll | 34/34 lines, 28/28 host branches. F(lifecycle + `kill`/`uevent 19`/`spawn WATR`): no ragdoll, creation/timer expiry, unprimed/primed, invalid fluid/wrong type/inactive/active water, over/continuing/fading/alpha conditions. Real physics Update runs before its over/continue result is forced. 0x193C–44 uses inclusive alpha >=1; 0x19A4/19A8 uses timer >0; 0x19B8–C0 uses timer <=0: unchanged NaN behavior. |
| 6284 | GetDamageVulnerability | 6/6 branches. F(lifecycle + four `damage` calls): warp-in, warp-out, shadow dash, ordinary vulnerability. |
| 62D0 | KnockBack | 21/21 lines, 24/24 branches; 13 F(knock spec + `damage`) cases, 65/65 assertions PASS. Covers existing ragdoll, knocked/dashing/alive gates, freeze, flinch/knock/hurl, dead non-hurl, four suppressed death followups and audible death. Checks all EnableAnimReaction arguments, base-call count, flags, freeze calls and sound-call count. Real base KnockBack runs; its reaction/followup outputs are replaced afterward to isolate this REL's logic from the DOL RULE evaluator. |
| 6528 | PreRender | 16/16 branches. F(lifecycle): absent/unprimed/primed ragdoll, fade/warp flags, ordinary visibility. Retail warp bits at 0x658C/6594 and base fade flag agree. |
| 6664 | Render | 10/10 branches. F(lifecycle): alive/dead, fading/not, alpha byte 0/128/255; both draw paths. Retail 0x66D4/D8 and 0x670C/10 are plain >0 comparisons, preserved. |
| 6748 | DoUserAnimEvent | 16/16 branches. F(lifecycle + `uevent`): projectile EMP/beam/neither, dash begin/end on/off, animation end, ragdoll event, unhandled event/base fallback. |
| 68B4 | Think | 12/12 branches. F(lifecycle): active/inactive, ragdoll priming, live/dead and frozen/unfrozen; one frozen result reapplied after real base Think to reach the subsequent bone-tracking gate. |
| 6A48 | AcceptScriptMsg | 8/8 branches. F(`send` and lifecycle): activation/deactivation/delete/escape, OffGround frozen/unfrozen; base handler still runs. |
| 6FA0 | CDarkCommando ctor | 45/45 lines, 28/32 reported non-throw edges; all normal-return branches ran. F(loader spec) invalidates the first mold asset temporarily; first actor retains an absent optional, second retains a real token. Four untaken edges descend solely from exception-cleanup blocks 42/53 in the original gcov metadata. Member copies are implicit, not missing implementations (pending marker treatment below). |

### Lifetimes and ABI qualifications

- Retail A80/DB4/1078/1460 are MWCC destructors with `this==0` and signed deleting-flag
  guards. All direct call sites pass `r4=-1` and nonnull stack/member addresses: 0x398, 0x498,
  0x848, 0xAA8, 0xAB4, 0xAC0, 0x6C94, 0x8DD8. The module's text/data references contain no
  address-taken use of those wrappers; 820 has no references at all. Live stack teardown is
  inlined in the loader. Therefore the wrapper's null/deleting arms are dead for these lifetimes,
  while the host ordinary destructors and automatic member cleanup reproduce the live work.
- Default constructors and the main ctor contain host exception-cleanup landing pads not present
  in the retail body. Raw gcov (including `throw:true` edges and block ids) distinguishes them
  from gameplay branches. Do not interpret 28/32 as four missing ordinary ctor decisions.
- Retail `__nw` null guards are **not proven dead retail branches**. Host `rs_new` is ordinary
  throwing `new` (`include/Kyoto/Alloc/CMemory.hpp:52–63`), so those null-return paths cannot be
  forced by normal host allocation failure. The new markers cover the host normal-return paths;
  retail OOM behavior remains an explicit ABI limitation, also applicable to the older decoy/beam
  allocation markers. No fake null result was injected into throwing `new`.

### Earlier-marker coverage strengthened

| bodies | forced setup and outcome |
|---|---|
| GetUp / WarpOut | `statecall` messages 0/1/2 plus lifecycle spec: GetUp 7/8 (only invalid EStateMsg final else untaken; retail enum is 0..2), WarpOut 14/14. Ready/not-ready command gate; generation/not-generation; cloak alpha; frozen; deletion tail all ran. |
| Null target paths | Lifecycle spec clears target for SelectMeleeVariant, ShouldMeleeAttack, SetTargetFaceVect, SelectDashTarget, BuildActionChoices, LaunchGrenade, FireChargeBeamShot and UpdateAdditiveAim. Attack spec covers MeleeAttack, FireChargeBeam, FireEMP before/after warmup. These are forced executions, not dead-code claims. |
| MeleeAttack / FireEMP / FireChargeBeam | Grand Puzzle layer 8, real team manager: temporary capacity/time inputs produce no-team, permitted and denied Starts for all three states; **9/9 readiness assertions PASS**, plus corresponding EndAttack permutations. Only MeleeAttack's variant result is forced to 0 after its real selector returns, to isolate team permission. MeleeAttack 15/16 and FireChargeBeam 13/14 have only invalid-message else untaken; FireEMP retains angle/valid-target gaps (20/24). |
| ChargeBeam Think / collision | Real projectile/mold lifecycle, safe-zone query returns forced false/true after real calls; timer >=5, invalid player/effect paths. Think 16/16. Collision ran both safe-zone outcomes but remains 14/20; this is not full collision coverage. |
| CShadowDecoy | Missing effect, zero health, forced deletable result, ordinary update and Delete emitter cleanup. Think 9/10: redundant final effect-null arm is dead by the dc3 same-pointer/no-intervening-writer proof; AcceptScriptMsg 4/4. Visor attempts include dark/echo/combat and null effect, but AddToRenderer remains 7/8: one live visor arm still lacks proof. |
| UpdateCloak | 16/16 in the combined dc4 runs, including real combat/dark visor switches. |

Other previous-marker gaps remain, including collision optional/token gates, beam ctor optional
copy, several attack geometry/aim/choice paths, and decoy constructor effect-null. Counts in the
retained coverage table are the authoritative scope of these runs, not evidence that older markers
were all exercised. The four remaining **B** rows are `7880`, `797C`, `7D40`, `7DDC`: implicit
copies of main/shadow/charge/EMP data, respectively. Each compiler-generated retail copy is
100% matching and copies run through actor/projectile construction. There is no explicit source
body to place a conventional marker above. Their B classifications are unchanged; marker versus
host-copy treatment is left to the orchestrator. No artificial copy implementation was introduced.

### Reproduction and retained evidence

Run `tools/games_running.sh --wait` before each; all use the coverage-enabled `build-port-link`:

    MP2_FAST_LOAD=1 SECS=360 tools/cov/force.sh tools/cov/specs/darkcommando_dc4.py <knock> \
      3bfa3eff,06_temple_sacredpath_dark --con tools/drivers/darkcommando_dc4.con --frames 2000 --fast
    MP2_FAST_LOAD=1 SECS=360 tools/cov/force.sh tools/cov/specs/darkcommando_dc4_loaders.py <loaders> \
      3bfa3eff,06_temple_sacredpath_dark --con tools/drivers/darkcommando_dc4_lifecycle.con --frames 2400 --fast
    MP2_FAST_LOAD=1 SECS=360 tools/cov/force.sh tools/cov/specs/darkcommando_dc4_attacks.py <attacks> \
      3bfa3eff,11_temple_grandpuzzle --con tools/drivers/darkcommando_dc4_attacks.con --frames 2700 --fast

Successful original runs: knock (2000 updates), loaders (2400), attacks2 (2700); final loader
rerun (2400) adds the top-level `.inc` default arm. All exited normally; `[port] caught` /
`reach-stub` / Python exceptions / failed assertions **0**. Sacred Path is area 55; Grand Puzzle
is area 29. Artificial entry inputs generally restore at return; injected dependency outputs
and deliberate object lifecycle changes can persist for the run. No source probes.

Evidence: `/tmp/opencode/reports/dc4-evidence/` contains successful game/console/forcing logs,
the 65-check list, coverage summary, raw JSON, asm/check logs and matching-module report.
Original `.gcno` plus `knock.gcda`, `loaders.gcda`, `attacks2.gcda` belong together; the final
loader rebuild has a separate `loaders-final.gcno`/`.gcda` pair. Rename each copied pair to
`CDarkCommando.cpp.gcno`/`.gcda` in scratch when running gcov. Never merge the two build cohorts.
The original `coverage.txt` was generated after marker comments shifted source lines: identify
rows by demangled function name, **not** its stale/missing retail-id column. Loader `.inc` and
header coverage must be inspected in raw JSON; the CPP-only summary omits them.

### Check findings and final gates

- Host `ninja -C build-port-link metroid_prime2_port`: link completed, **0 FAILED**.
  `gen_files_cmake.py --check`: rc 0. All four new Python specs compile syntactically.
- TARGET_PC strip-diff: **CLEAN**; stronger check confirms all three edited source/header files
  are byte-identical to base after removing marker comments, in both PC and non-PC paths.
  `configure.py` and matching ninja succeeded: All **77.98% fuzzy / 54.39% matched**, Game Code
  **94.14% / 63.29%**. Explicit `build/tools/dtk shasum -c config/G2ME01/build.sha1`:
  **87 entries OK**, rc 0 (the incremental ninja run did not need to repeat CHECK).
- `asmcheck` all tags: CPP 12 bodies / 48 findings (+ NOPOS), loader header 13 / 77.
  Getter's one-line inline definition is not parsed (`MISSING fn_3_8`, 0 findings): manually
  checked rodata 50 and 100% match. SetFuncPtrs is branch-free/100% matching and runtime
  registration is established above. No unresolved gameplay divergence in these findings.
- BIT/NOPOS: use dc3's 0xcb4/0xcb5 mapping. Ctor 0x732C–0x73B4 clears all twelve named flags;
  Escape sets warp-request, events test dash/EMP/beam and set dash-moving, Render tests fading,
  PreRender tests warp flags, KnockBack tests dash/knocked and sets knocked/hurled, vulnerability
  tests warp/dash. The checker's offset-named fields are absent because upstream uses named
  bitfields. Base fade-to-death is 0x420 mask 0x10 (`0x1950 rlwimi r0,r5,4,27,27`); all tests/stores agree.
- CPP CALL/BRANCH/CALLS: constructor initializers implement bone tracking, projectile info,
  vulnerability and main-data copies; temporary strings clean themselves up. Ctor optional
  and copied vulnerability containers account for conditional work hidden from the lexical
  checker. `TCastToConstPtr<CScriptWater>` and retail mutable cast perform the same lookup/type
  test. Ragdoll `fn_3_1AD0` is the actual `single_ptr` assignment. Loader default constructors,
  `CColor` defaults, member destructors and optional model copy are automatic C++ work;
  `.inc` supplies the parsers/ReadBytes; LdrToActorParameters/LdrToTransform4f are the named
  equivalents of retail LoadActorParameters/LoadEditorTransform.
- Loader-header XSTORE: every cited `x<8-digit>` value is a **property hash comment**, not an
  object byte offset (the checker even sums nested hashes). No stores were added. Actual
  constructor offsets/defaults agree with the audited retail initializer sequences, including
  dc1's PC defaults. CALL/CALLS: damage/audio/vulnerability constructors and member destructors
  are implicit; LoadTypedefSLdrDamageInfo is named LoadTypedefDamageInfo here. `Free` is the
  unreachable deleting-destructor wrapper arm covered by the lifetime proof above.
- `retail_diff` (25 parsed new bodies): 3 call and 3 constant leads. Loader `89C` is in `.inc`,
  `4D8` is the optional-model copy; 360 is `patterned.turnSpeed` in the loader header.
  Ragdoll `1AD0` is `single_ptr` assignment. PreRender 255 and Render 1/255 are inlined CColor
  float/byte conversions. No compare/NaN changes.
- `review_decomp_diff.sh <base>`: the 26 MARK entries are resolved by the per-body table and
  qualifications above. No gameplay code added/removed. The script's ASM pass selects only
  Port*.cpp; explicit upstream-named CPP/header checks above supply the required review.

## dct1 offline survey (the four open B rows are compiler-generated copies)

Base `234139ed2`, branch `decomp/dct1`. **No game launched, no code changed.** dc4 left four B
rows open — `fn_3_7880`, `fn_3_797C`, `fn_3_7D40`, `fn_3_7DDC` — and the TSV calls them
`SLdr*Data (copy)`. They are not missing bodies: each is the compiler-generated copy constructor
of a loader struct the module instantiates, and the port gets the same constructor implicitly.
Retail source: `CDarkCommando.s` of a matching build.

| fn_3_ | struct | retail body | calls |
|---|---|---|---|
| 7DDC | `SLdrDarkCommandoEMPData` | 0x7DDC-0x7E6F, branch-free. Copies +0x00-0x44 word by word: three floats, the four `grenadeDamage` words (0xC/0x10/0x14/0x18: `dI_WeaponType` as a word, damage/radius/knockback as `stfs`), the three CAssetIds 0x1C/0x20/0x24, four floats 0x28-0x34, 0x38 int, 0x3C `stfs` (eMPDuration), 0x40/0x44 the two sound ids — the 15 declared members of `SLdrDarkCommando.hpp:14`, in declaration order, ending at sizeof 0x48. | called from the grenade ctor 0x9094 |
| 7D40 | `SLdrDarkCommandoChargeBeamData` | 0x7D40-0x7DDB, branch-free. Plain words +0x00-0x48 ending in the bool at 0x48 (sizeof 0x4C): 3 floats, `damage` 0xC-0x18, `projectile` 0x1C, `moldEffect` 0x20, `moldDamage` 0x24-0x30, `sound_Mold` 0x34-0x48 (int, float, three words, bool) — the 8 declared members. | copies go through the main copy at +0x98 |
| 797C | `SLdrDarkCommandoShadowDashData` | 0x797C-0x7A83. Plain words +0x00-0x58 (4 floats, `shadowDecoyFx` 0x10, the three `SLdrAudioPlaybackParms` at 0x14/0x2C/0x44 with their trailing bools at 0x28/0x40/0x58), then two `SLdrDamageVulnerability` copy calls at +0x5C and +0x1B8 (so that member is 0x15C and non-trivial). | main copy at +0xE4 |
| 7880 | `SLdrDarkCommandoData` | 0x7880-0x797B. Plain words +0x00-0x4C (4 chance floats, `bladeDamage` 0x10-0x1C, `sound_ImpactRagDoll` 0x20, `sound_HurledDeath` 0x38), then the three nested copies at +0x50 (EMPData), +0x98 (ChargeBeamData), +0xE4 (ShadowDashData) — exactly the 10 declared members. | `CDarkCommando` ctor 0x7018 copies the loader data into the actor |

Why this is offline-settled: each retail body contains no branch at all — it is the memberwise copy
in declaration order, with the same load/store width per member and the same nested-copy calls for
the members that have non-trivial copy constructors. That is exactly what the host compiler emits
for the same member list (which the already-marked ctors `fn_3_B14`/`E3C`/`10E8`/`14B8` and the
loader property tables establish), since none of the four structs declares a copy constructor of
its own. There is therefore no arm to run and no new runtime evidence can add anything: the rows
are complete once someone decides what a "marker" means for a body nobody wrote.

**dct2: marked (decision taken).** The orchestrator chose to mark them, on the precedent of
`include/MetroidPrime/Enemies/CShrieker.hpp:25` and
`src/MetroidPrime/Enemies/PortCBoostBallGuardianBall.cpp:1106-1107` (`// @ fn_..` above the struct
with a proof comment for the implicit copy). Each of the four now carries a marker plus a slot map
immediately above its declaration in `include/MetroidPrime/ScriptLoader/SLdrDarkCommando.hpp`.
Comment-only: no code change.

Per-struct mapping check (retail copy body vs the declared member list, slot by slot; "w" = a 4-byte
word copied with `stw`, "f" = a `stfs` float, "b" = a `stb` bool; a store's source and destination
offsets are identical in all four bodies, so it is a straight same-offset memberwise copy):

| fn_3_ | struct | slots | slot map |
|---|---|---|---|
| 7DDC | `SLdrDarkCommandoEMPData` | 18 | 3 floats, `grenadeDamage` (w + 3 f), 3 CAssetIds, 4 floats, w, f, 2 w = the 15 members; sizeof 0x48 both sides |
| 7D40 | `SLdrDarkCommandoChargeBeamData` | 19 | 3 floats, `damage` (w+3f), 2 CAssetIds, `moldDamage` (w+3f), `sound_Mold` (f,f,w,w,w,b) = the 8 members; sizeof 0x4C |
| 797C | `SLdrDarkCommandoShadowDashData` | 25 | 4 floats, `shadowDecoyFx`, 3 `SLdrAudioPlaybackParms` (6 slots each), then 2 nested `SLdrDamageVulnerability` copy calls = the 10 members; sizeof 0x314 |
| 7880 | `SLdrDarkCommandoData` | 23 | 4 floats, `bladeDamage` (w+3f), 2 `SLdrAudioPlaybackParms`, then nested-child copies at +0x50/+0x98/+0xE4 = the 10 members; sizeof 0x3F8 |

No retail slot is left without a declared member (no padding word or byte is copied) and no declared
member is left uncopied; every nested copy call's type is the type of the member at that position.
The host layout was checked, not assumed: a `-fsyntax-only` TU built with `build-port-link`'s own
flags asserts `sizeof` 0x48/0x4C/0x314/0x3F8, `SLdrAudioPlaybackParms` 0x18,
`SLdrDamageVulnerability` 0x15C (29 weapon entries) and the offsets used above
(`eMPDuration` 0x3C, `sound_Mold` 0x34, `shadowDecoyFx` 0x10, `sound_Cloak` 0x2C,
`shadowDecoyVulnerability` 0x5C, `shadowDashVulnerability` 0x1B8, `bladeDamage` 0x10,
`sound_HurledDeath` 0x38, `eMPGrenadeAttackInfo` 0x50, `chargeBeamAttackInfo` 0x98,
`shadowDashInfo` 0xE4).
