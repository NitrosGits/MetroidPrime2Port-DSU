# SnakeWeedSwarm REL (module 71): analysis for a host body

Analysis + host body, milestone 1 (2026-10-16). Sources: `config/G2ME01/rels/SnakeWeedSwarm/symbols.txt`
(sizes), the archive build's disassembly `../MetroidPrime2Port/build/G2ME01/SnakeWeedSwarm/asm/`
(`auto_00_00000000_text.s` is the complete `.text`; `auto_00_000000DC_text.s` starts at the loader;
`auto_03/04/05_*` are rodata/data/bss), the generated
`include/MetroidPrime/ScriptLoader/SLdrSnakeWeedSwarm.{hpp,inc}`, and MP1's
`../MetroidPrimePort/src/MetroidPrime/Enemies/CSnakeWeedSwarm.cpp` (+ its header) for what the class
*does*. Confidence tags as in `SpacePirate.md`: **[proven]** = read from an instruction/table entry;
**[probable]** = strong evidence but not proven; **[?]** = guess. All `fn_71_X` are REL `.text`
offsets. The implementation is `src/MetroidPrime/Enemies/PortCSnakeWeedSwarm.cpp` and the
`TARGET_PC` half of `include/MetroidPrime/Enemies/CSnakeWeedSwarm.hpp`.

**Shape in one line:** a `CActor` (not `CPatterned`), and structurally MP1's `CSnakeWeedSwarm` with
every member at MP1's offset + 0x70 (Echoes' `CActor` is 0x158, MP1's 0xe8). No body controller, so
none of the patterned-actor activation the other families need.

## 1. Function map

| fn | size | role | evidence |
|---|---|---|---|
| `RELMain` | 0x20 | calls `fn_71_70` | [proven] |
| `fn_71_70` | 0x6C | stores loader `fn_71_DC` at `lbl_71_bss_40`+0, copies PMF `lbl_71_data_18` to +0x4 and `lbl_71_data_24` to +0x10 (0x1c table = `SSnakeWeedSwarm_FuncPtrs`), then `SetLoader_SnakeWeedSwarm` (the DOL's `SetSSnakeWeedSwarm_FuncPtrs`) | [proven] |
| `RELExit` | 0x24 | `SetSSnakeWeedSwarm_FuncPtrs(nullptr)` | [proven] |
| `fn_71_0` | 0x2C | `GetHealthInfo` (weak inline emitted in the REL): `bctrl` through vtable +0x38 = slot 14 `HealthInfo` | [proven] vtable slot 15 + body |
| `fn_71_DC` | 0x544 | **loader** `LoadSnakeWeedSwarm` (section 3) | [proven] bss_40+0 |
| `fn_71_2B84` | 0xB54 | **constructor** | [proven]: called right after `rs_new 0x298` (@0x5D0) |
| `fn_71_2810` | 0x1B0 | **destructor** (tokens, `fn_71_2B2C`/`2AD4`/`2A44`/`29C0` member dtors, `CSkinnedModelState`, `~CActor`) | [proven] vtable slot 2 |
| `fn_71_1E58` | 0x4D4 | **`Think`**: `CActor::Think` first, then `fn_71_133C`/`fn_71_D10` (swarm creation), `CAnimData::SetPlaybackRate`, `CModelData::AdvanceAnimation`, boid timers via the accessors, `CSfxManager::AddEmitter`/`RemoveEmitter`, `CRandom16::Float` | [proven] slot 5 + calls |
| `fn_71_274C` | 0xC4 | **`AcceptScriptMsg`** (body in section 4) | [proven] slot 6 + body |
| `fn_71_1934` | 0x1C4 | **`PreRender`**: `CFrustumPlanes::BoxInFrustumPlanes`, `CAnimData::PreRender`, `CActorLights::BuildAreaLightList`/`BuildDynamicLightList` (no `CActor::PreRender`) | [proven] slot 9 + calls |
| `fn_71_163C` | 0x184 | **`AddToRenderer`**: `ActivateLights`, `GetRenderAlphaBufferAlpha`, `fn_71_17C0` per boid (no `CActor::AddToRenderer`) | [proven] slot 10 + calls |
| `fn_71_17C0` | 0x174 | `RenderBoid` (MP1 name) | [probable] call site + MP1 |
| `fn_71_2494` | 0x54 | **`GetTouchBounds`**: if bit 0x80 of byte 0x1b0 (`extrwi. r0,r0,1,24`) return the box at 0x1b4..0x1cb, else none | [proven] slot 18 + whole body |
| `fn_71_1CC0` | 0x198 | **`Touch`**: `TCastToPtr<CGameProjectile>` -> `fn_71_1B3C`; `TCastToPtr<CPlayer>` -> `SendScriptMsg` (contact damage) | [proven] slot 19 + calls |
| `fn_71_1AF8` | 0x44 | **`ApplyRadiusDamage(pos, info, mgr)`**: `lhz 0x0(info)` == 4 or 5 (bomb/power bomb) -> `fn_71_1B3C(this, mgr, pos, lfs 0xc(info))` | [proven] PMF `lbl_71_data_18` + whole body |
| `fn_71_1B3C` | 0x184 | **`ScareSnakeWeeds(mgr, pos, radius)`**: boid loop, `CRandom16::Float`, `AddEmitter` | [proven] PMF `lbl_71_data_24`; signature from 1AF8's call and the table's member type |
| `fn_71_256C` | 0x190 | `AllocateSkinnedModels(mgr, which)` (MP1 name): `PickAnimatedModel`, `MakeDefaultStorage`, `EnableLooping`, `GetAnimTimeRemaining`, `AdvanceAnimation` | [probable] calls + MP1; called from kSM_Create with 0 |
| `fn_71_133C`, `fn_71_D10` | 0x238, 0x274 | swarm creation / flood fill (MP1 `CreateSwarm`/`FloodFill`) | [?] from Think's call order + MP1 |
| `fn_71_7F0`, `fn_71_880` | 0x90 each | particle adders (MP1 `AddContinuousParticles`/`AddRetreatParticles`) | [?] |
| `fn_71_39B8`..`fn_71_39FC` | 0x4-0x8 | `CBoid` one-instruction accessors | [probable] sizes + call density |
| `fn_71_3CF4` | 0x50 | dtor of the temporary `auto_ptr<CModelData>` in the ctor's four model slots | [probable] |

### Vtable `lbl_71_data_30` (31 words, 2 leading zeros)

Slot numbers count the two header words, as in the other analyses. Overridden (REL address):
2 dtor `fn_71_2810`, 3 `CSnakeWeedSwarm::TypesMatch` (the DOL's), 5 Think `fn_71_1E58`,
6 AcceptScriptMsg `fn_71_274C`, 9 PreRender `fn_71_1934`, 10 AddToRenderer `fn_71_163C`,
15 GetHealthInfo `fn_71_0`, 18 GetTouchBounds `fn_71_2494`, 19 Touch `fn_71_1CC0`.
Inherited (DOL symbols in the slot): 4 `CEntity::PreThink`, 7 `CActor::SetActive`, 8 `ClearFluidList`,
11 `CActor::Render`, 12 `CanRenderUnsorted`, 13 `PreRenderAllViewports`, 14 `HealthInfo`,
16/17 `GetDamageVulnerability`, 20-32 `GetOrbitPosition` .. `ProcessSoundEvent`.

## 2. CSnakeWeedSwarm members (0x298, the loader's `rs_new`)

All from the constructor `fn_71_2B84` unless noted.

| off | member | evidence |
|---|---|---|
| 0x000 | `CActor` base: `(uid, name, info, 0, CTransform4f::Translate(pos), CModelData(animRes), CMaterialList(kMT_Trigger, kMT_NonSolidDamageable), actParms, kInvalidUniqueId)` | [proven] call; materials from `lbl_71_data_4` = 0x22, `lbl_71_data_8` = 0x32 via `__shl2i`, 0x3b (= kMT_Solid) as end marker in `data_0/C/14` |
| 0x158 | `CVector3f` scale | [proven] stores of f1/f2/f0 (the loader's r8) |
| 0x164..0x198 | 14 floats (0x18c = `cos(maxSlope * lbl_71_rodata_50)`, rodata_50 = 0.017453292) | [proven] stores; [probable] names, MP1 order |
| 0x19c | uint | [proven] zeroed |
| 0x1a0 | `rstl::vector<CBoid>` | [proven] 0x1a4/0x1a8/0x1ac zeroed; [probable] type, MP1 |
| 0x1b0 | flag byte (bit 0x80 = has bounds), 0x1b1 byte | [proven] |
| 0x1b4 | `CAABox` touch bounds (from `CAABox::mskInvertedBox`) | [proven] |
| 0x1cc | `CDamageInfo` (0x1c) | [proven] store pattern int/4 floats/3 shorts/byte |
| 0x1e8, 0x20c | position / normal workspaces (`reserved_vector`, count words zeroed) | [probable] MP1 |
| 0x22c | `reserved_vector<ncrc_ptr<CModelData>,4>`: four `rs_new CModelData(animRes)` + `rs_new int(1)` refcounts | [proven] four `__nw 0x4c`/`__nw 4` blocks |
| 0x250 | which-model | [probable] |
| 0x254, 0x258 | `single_ptr<vector<...>>` boid positions / placement (`rs_new 0x10`) | [proven] stores |
| 0x25c/0x25e/0x260 | three sfx ids (each loaded via DOL `lbl_8041E2E6`, probably `kInternalInvalidSfxId` translation of the loader's ids) | [proven] `sth`; [?] the DOL symbol |
| 0x264 | `CSfxHandle` looped emitter | [proven] removed on XDEL |
| 0x268/0x270 | optional `TLockedToken<CGenDescription>` (bool + ptr), loaded as `'PART'` (0x5041...) when the id is valid | [proven] |
| 0x278/0x280 | optional `CElementGen` (`rs_new 0x338`) | [proven] |
| 0x288, 0x28c, 0x290 | particle count (r28), float (f31), timer | [proven] stores |

The ctor ends with `MakeActorLights` from the actor parameters + `SetActorLights` [proven].

## 3. Loader `fn_71_DC`

Reads the 22 property ids of `SLdrSnakeWeedSwarm` (the `lis` immediates match the generated `.inc`
id by id: 0x255a4580, 0xe25fb08c, 0x7e397fed, 17 floats, 0xd756416e damage, three sound ints), then
`rs_new 0x298` and the ctor with: r4 uid, r5 name (`r1+0xe8`, the editor properties), r6
`LdrToEntityInfo`, r7 position (`r1+0xf8`), r8 scale (`r1+0x110`), r9 a stack `CAnimRes` at `r1+0x50`
(ancs, character, scale `1,1,1` from `lbl_71_rodata_0`, loop byte 1, initial anim), r10
`LdrToActorParameters`; on the stack the floats, the `LdrToDamageInfo` result, the three sound ids,
and then **-1, 0, -1, 0.0f** - i.e. Echoes' properties carry no particle assets: both particle ids
are invalid and the count is 0, so the token/`CElementGen` members stay empty [proven: `li r11,-1`,
`li r0,0`, stores at 0x34..0x40]. The retail loader does not check the ANCS; the host one skips an
object with no `ANCS` and logs it.

## 4. Host body (milestone 1) - real vs stand-in

Real (decided by the disassembly): the loader; the constructor's base call, scale, floats,
`cos(maxSlope)`, bounds, damage and sound ids; `GetTouchBounds`; `ApplyRadiusDamage` (bomb/power-bomb
filter, then `ScareSnakeWeeds(mgr, pos, info.GetRadius())`); `AcceptScriptMsg` (`CActor` first; on
`kSM_Create` = 'XCRT' `SetCalculateLighting(true)`; on `kSM_Delete` = 'XDEL' remove the emitter);
the module table (loader + both pointers-to-member, `static_cast` to the `CEntity` bridge type).

Stand-ins (once-per-method `[snakeweed] ... is a port stand-in` line): `Think` (`CActor::Think`
only), `PreRender` / `AddToRenderer` (forward to `CActor`, so the swarm draws its own model once at
its origin instead of a field of boids), `Touch` (forwards to `CActor`; unreachable while the
bounds flag is clear), `ScareSnakeWeeds` (no boids), and `fn_71_256C` from `kSM_Create`. Not held:
boids, model slots, workspaces, particles, the actor-lights cast-shadow bit set on create
(`lwz 0xbc` -> `+0x2a0` bit 0x40). No contact damage is ever dealt.

## 5. Where its actors are

From the disc's MLVLs (`/tmp/module_tally.py` + a per-layer query of each area's `mRelOffsets`):
**Temple Grounds `3bfa3eff` area 40**, and **`3dfd2249` areas 2 and 51** - and in all three the module
belongs to **layer 0**, so its actors load with the area and need no layer activation. TG 40 is in
the save's own world: `MP2_TELEPORT=40` reaches it.

**Verified in game (2026-10-16, later session).** Booting straight into `3bfa3eff` area 40
(`MP2_WORLD="3bfa3eff,688ab8ae"`, `MP2_SKIP_CINEMATICS=1`) puts the game in `[area] 40` and the host
body runs: `[snakeweed] CSnakeWeedSwarm::AllocateSkinnedModels (fn_71_256C, from kSM_Create)`,
`Think` (slot 5), `PreRender` (slot 9) and `AddToRenderer` (slot 10) each report their stand-in, i.e.
the loader creates the actor, `kSM_Create` reaches the class, and the draw path calls it every frame
(`/tmp/runD.log`). The module is no longer an entry-point-only stub, and no run faulted. What the log
also shows as absent is what section 4 lists: the boids themselves never exist, so there is one
model at the origin and no contact damage.
