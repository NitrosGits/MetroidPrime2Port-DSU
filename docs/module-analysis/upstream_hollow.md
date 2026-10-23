# Upstream hollow bodies (audit stub1, 2026-10-23, base bbbf6584d)

Which functions the port compiles from PrimeDecomp's sources (66 `upstream` REL modules in
`docs/modules.tsv`, plus the DOL units `files.cmake` builds) are skeletons of retail, or are not
defined upstream at all and resolve to a reach stub.

## Rerun

    python3 configure.py && $T/ninja      # matching build: build/G2ME01/report.json + .o files
    python3 tools/upstream_hollow.py --dol --md /tmp/upstream_hollow.md
    python3 tools/upstream_hollow.py --markers-from ../wt-mp2-mi3 MediumIng   # what a lane branch still leaves

Runs in ~2 s (REL only) / ~7 s (`--dol`). Signals and thresholds: see the tool's docstring
(retail >= 0x40 bytes; EMPTY = source body empty / `return <const>;` or compiled <= 8 bytes;
TINY = compiled < 25% of retail; LOWMATCH = objdiff fuzzy < 10%; STUB/MISSING = no upstream
definition; UNPAIRED = unnamed retail fn with no upstream counterpart, after size pairing and
dropping dead or inlined ones). TARGET_PC bodies count as the port's (listed, not flagged).
`--min 0x10` shows the small ones (TypesMatch, string operator==, loader typedefs).

## Result

- **MediumIng is the only hollow REL module**: 42 named bodies (41 EMPTY, 1 TINY) + 10 unnamed
  helpers (fn_41_6078 0x6E8, 4644, 8CB4, 49CC, 6F18, 4470, 6BFC, 4308, 4598, 9C8C, 3064...),
  ~22.8 KB of 45.5 KB. With `--markers-from ../wt-mp2-mi3` (a3a4c03f7) 19 carry markers, 33 remain
  (markers only count behaviour-checked bodies; mi3 may have unmarked bodies).
- The other 65 upstream modules: no EMPTY/TINY/STUB/MISSING body >= 0x40. Their low coverage
  (Rezbit 26%, AIMannedTurret 37%...) is unreached arms, not missing code. CMetroid
  ApplyDamageGrowth/PreDamageSpacePirate (up1's reach stubs) now have bodies.
- DOL holes (resident, every module that uses them):
  - `CBloggProjectile::ApplyDamageToOneActor` fn_80213508 (0x68): upstream body empty
    (`CBloggProjectile.cpp:15`); retail calls `CGameProjectile::ApplyDamageToOneActor` (0x80213530)
    and then only side-effect-free casts. **A Blogg shot's direct hit applies no damage** (`CGameProjectile.cpp:450` dispatches to the
    empty override).
  - `CDamageInfo::WithNoImmunity` fn_800B5F8C (0x64): reach stub (reachstub_311) that leaves the
    returned CDamageInfo unwritten; `CParasite.cpp:437` passes it to `ApplyDamage` (contact damage
    while x944_ > 0) = uninitialised damage. Retail: copy all fields, set byte 0x1A bit 0x80
    (`rlwimi r0,r5,7,24,24`, r5 = 1), keep the source's 0x40 bit (`rlwimi r0,r6,0,25,25`).
  - Weak (LOWMATCH, judge before porting): `CVirtualBone::BuildSkinningMatrices` fn_80310838
    (5.7%), `LdrToDamageInfo` (0.9%, read: same fields as retail, fine), CGunTurretTop
    GetTouchBounds (0x3C, 4.5%).
- Small (`--min 0x10`): `rstl::operator==(const string&, const char*)` reach stub (reachstub_178,
  CAutoMapper; returns garbage bool), CMetareeSwarmC/CPillBugC::TypesMatch and
  LoadTypedefMasterLayer absent (no references found / inlined), IngSnatchingSwarm fn_33_4F44
  (particle draw-mask predicate in table lbl_33_data_2E0).
- Already TARGET_PC-ported (GameCube side empty): CAnimData::DrawSkinnedModel, CMorphBall::Render.

## Port slices (most visible first)

1. **MediumIng remainder** (mi3 follow-up): what `--markers-from ../wt-mp2-mi3` still flags
   (33 ids on a3a4c03f7; the table below is the base, all 52). Agon Wastes/04_sand_mining_dark[L5]. Merge mi3 first; split into
   states (MeleeAttack, ArmAttack, MistAttack, MistIn/Out, Dash, SafezoneReaction, MoveToSafeZone,
   Jump, Taunt, BackUp, Dead), movement (SetDash/Mesh/Retreat destination, SelectTarget,
   SetLeashTarget, PathFind, Follow*Pattern, CalculateSeparation, SubStart, WaitForLocomotion),
   finders (FindSafeZoneRetreatPoint, FindClosest*Actor) + their fn_ helpers.
2. **Blogg projectile damage**: CBloggProjectile::ApplyDamageToOneActor = base call (TARGET_PC
   block in `src/MetroidPrime/Weapons/CBloggProjectile.cpp:15`). Torvus Bog/07_swamp_shaft[L2,3,5];
   check the player takes damage from a Blogg shot. ~5 lines, orchestrator-sized.
3. **CDamageInfo::WithNoImmunity**: real body in `src/MetroidPrime/CDamageInfo.cpp` (TARGET_PC),
   delete reachstub_311. Agon Wastes/01_sand_entrance[L3] (Parasite contact damage). Small.
4. **string operator==** (reachstub_178): find its port callers (CAutoMapper) and define it in
   the rstl header TARGET_PC block. Small.
5. Review-only: CVirtualBone::BuildSkinningMatrices vs fn_80310838; fn_33_4F44 has an
   upstream equivalent?

## Generated table (base bbbf6584d, `--dol --top 120`)

Threshold: retail >= 0x40 bytes. Binary for MISSING: `/run/media/odran/Leo/projects/Restored-projects/Chatgpt/wt-mp2-port/build-port-link/metroid_prime2_port`. Coverage: up1/up2 reports.

## Modules (by hollow bytes)

| module | retail bytes | flagged fns | hollow bytes (est.) | % | of which UNPAIRED fn_ (n / bytes) | reach |
|---|---|---|---|---|---|---|
| MediumIng | 45524 | 52 | 22808 | 50 | 10 / 4560 | spawned; module lines 44.6%, up2 45.1% |
| DOL:MetroidPrime | 1012576 | 2 | 182 | 0 | 0 / 0 | DOL (resident) |
| DOL:Weapons | 172776 | 1 | 100 | 0 | 0 / 0 | DOL (resident) |
| DOL:Animation | 152120 | 1 | 92 | 0 | 0 / 0 | DOL (resident) |
| IngSnatchingSwarm | 21816 | 1 | 46 | 0 | 1 / 92 | not spawned; module lines 0.2% |

## Functions (ranked by score; top 120)

| # | module | function | retail id / size | compiled | upstream | flag | why | reached |
|---|---|---|---|---|---|---|---|---|
| 1 | MediumIng | `CMediumIng::AcceptScriptMsg(CStateManager&, const CScriptMsg&)` | fn_41_79C8 / 0x644 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:597 | EMPTY | empty body; fuzzy 0.2% | spawned; module lines 44.6%, up2 45.1% |
| 2 | MediumIng | `CMediumIngTentacle::PreRender(CStateManager&)` | fn_41_94C8 / 0x560 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:608 | EMPTY | empty body; fuzzy 0.3% | spawned; module lines 44.6%, up2 45.1% |
| 3 | MediumIng | `CMediumIng::MeleeAttack(CStateManager&, EStateMsg, float)` | fn_41_3AE4 / 0x458 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:578 | EMPTY | empty body; fuzzy 0.4% | spawned; module lines 44.6%, up2 45.1% |
| 4 | MediumIng | `CMediumIng::SetDashDestination(CStateManager&, float)` | fn_41_261C / 0x458 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:594 | EMPTY | empty body; fuzzy 0.4% | spawned; module lines 44.6%, up2 45.1% |
| 5 | MediumIng | `CMediumIng::ArmAttack(CStateManager&, EStateMsg, float)` | fn_41_3F3C / 0x3CC | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:577 | EMPTY | empty body; fuzzy 0.4% | spawned; module lines 44.6%, up2 45.1% |
| 6 | MediumIng | `fn_41_6078` | fn_41_6078 / 0x6E8 | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |
| 7 | MediumIng | `CMediumIng::MistIn(CStateManager&, EStateMsg, float)` | fn_41_5878 / 0x360 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:575 | EMPTY | empty body; fuzzy 0.5% | spawned; module lines 44.6%, up2 45.1% |
| 8 | MediumIng | `CMediumIng::CalculateSeparation(CStateManager&)` | fn_41_2024 / 0x304 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:632 | EMPTY | empty body; fuzzy 0.5% | spawned; module lines 44.6%, up2 45.1% |
| 9 | MediumIng | `CMediumIng::SafezoneReaction(CStateManager&, EStateMsg, float)` | fn_41_542C / 0x2F8 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:585 | EMPTY | empty body; fuzzy 0.5% | spawned; module lines 44.6%, up2 45.1% |
| 10 | MediumIng | `CMediumIng::MistAttack(CStateManager&, EStateMsg, float)` | fn_41_37B0 / 0x2D8 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:579 | EMPTY | empty body; fuzzy 0.5% | spawned; module lines 44.6%, up2 45.1% |
| 11 | MediumIng | `CMediumIng::UpdateMistEffect(CStateManager&)` | fn_41_1680 / 0x2B0 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:621 | EMPTY | empty body; fuzzy 0.6% | spawned; module lines 44.6%, up2 45.1% |
| 12 | MediumIng | `CMediumIng::SelectTarget(CStateManager&, float)` | fn_41_698C / 0x270 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:590 | EMPTY | empty body; fuzzy 0.6% | spawned; module lines 44.6%, up2 45.1% |
| 13 | MediumIng | `CMediumIngTentacle::Extend(CStateManager&, const CVector3f&)` | fn_41_9A34 / 0x258 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:613 | EMPTY | empty body; fuzzy 0.7% | spawned; module lines 44.6%, up2 45.1% |
| 14 | MediumIng | `CMediumIng::Dash(CStateManager&, EStateMsg, float)` | fn_41_4FE8 / 0x258 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:581 | EMPTY | empty body; fuzzy 0.7% | spawned; module lines 44.6%, up2 45.1% |
| 15 | MediumIng | `CMediumIng::MistOut(CStateManager&, EStateMsg, float)` | fn_41_4B8C / 0x234 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:576 | EMPTY | empty body; fuzzy 0.7% | spawned; module lines 44.6%, up2 45.1% |
| 16 | MediumIng | `CMediumIngTentacle::Touch(CActor&, CStateManager&)` | fn_41_91B0 / 0x210 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:612 | EMPTY | empty body; fuzzy 0.8% | spawned; module lines 44.6%, up2 45.1% |
| 17 | MediumIng | `CMediumIngTentacle::Retract(CStateManager&)` | fn_41_9CE0 / 0x1F0 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:614 | EMPTY | empty body; fuzzy 0.8% | spawned; module lines 44.6%, up2 45.1% |
| 18 | MediumIng | `CMediumIng::Touch(CActor&, CStateManager&)` | fn_41_761C / 0x1E8 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:600 | EMPTY | empty body; fuzzy 0.8% | spawned; module lines 44.6%, up2 45.1% |
| 19 | MediumIng | `CMediumIng::Think(float, CStateManager&)` | fn_41_7804 / 0x1C4 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:596 | EMPTY | empty body; fuzzy 0.9% | spawned; module lines 44.6%, up2 45.1% |
| 20 | MediumIng | `CMediumIng::Dead(CStateManager&, EStateMsg, float)` | fn_41_30B0 / 0x1B0 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:588 | EMPTY | empty body; fuzzy 0.9% | spawned; module lines 44.6%, up2 45.1% |
| 21 | MediumIng | `fn_41_4644` | fn_41_4644 / 0x34C | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |
| 22 | MediumIng | `CMediumIng::BackUp(CStateManager&, EStateMsg, float)` | fn_41_2E70 / 0x198 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:587 | EMPTY | empty body; fuzzy 1.0% | spawned; module lines 44.6%, up2 45.1% |
| 23 | MediumIng | `CMediumIng::MoveToSafeZone(CStateManager&, EStateMsg, float)` | fn_41_5240 / 0x180 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:586 | EMPTY | empty body; fuzzy 1.0% | spawned; module lines 44.6%, up2 45.1% |
| 24 | MediumIng | `CMediumIng::FindSafeZoneRetreatPoint(const CStateManager&, CVector3f&)` | fn_41_13E4 / 0x178 | 0x8 | src/MetroidPrime/Enemies/CMediumIng.cpp:617 | EMPTY | only `return false;`; fuzzy 1.5% | spawned; module lines 44.6%, up2 45.1% |
| 25 | MediumIng | `CMediumIng::Taunt(CStateManager&, EStateMsg, float)` | fn_41_4E1C / 0x170 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:582 | EMPTY | empty body; fuzzy 1.1% | spawned; module lines 44.6%, up2 45.1% |
| 26 | MediumIng | `CMediumIng::Jump(CStateManager&, EStateMsg, float)` | fn_41_5728 / 0x150 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:580 | EMPTY | empty body; fuzzy 1.2% | spawned; module lines 44.6%, up2 45.1% |
| 27 | MediumIng | `CMediumIngTentacle::Think(float, CStateManager&)` | fn_41_9F84 / 0x13C | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:607 | EMPTY | empty body; fuzzy 1.3% | spawned; module lines 44.6%, up2 45.1% |
| 28 | MediumIng | `CMediumIng::FindClosestDangerActor(const CStateManager&, const CVector3f&) const` | fn_41_1CE8 / 0x140 | 0x10 | src/MetroidPrime/Enemies/CMediumIng.cpp:629 | EMPTY | only `return kInvalidUniqueId;`; fuzzy 4.7% | spawned; module lines 44.6%, up2 45.1% |
| 29 | MediumIng | `CMediumIng::FindClosestRetreatActor(const CStateManager&, const CVector3f&) const` | fn_41_1BA8 / 0x140 | 0x10 | src/MetroidPrime/Enemies/CMediumIng.cpp:625 | EMPTY | only `return kInvalidUniqueId;`; fuzzy 4.7% | spawned; module lines 44.6%, up2 45.1% |
| 30 | MediumIng | `CMediumIng::CreateMistEffect(CStateManager&, const TLockedToken<CGenDescription>&)` | fn_41_198C / 0x128 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:622 | EMPTY | empty body; fuzzy 1.4% | spawned; module lines 44.6%, up2 45.1% |
| 31 | MediumIng | `CMediumIng::UpdateCurrentSafeZone(const CStateManager&, const CVector3f&)` | fn_41_155C / 0x124 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:620 | EMPTY | empty body; fuzzy 1.4% | spawned; module lines 44.6%, up2 45.1% |
| 32 | MediumIng | `CMediumIng::SetLeashTarget(CStateManager&, float)` | fn_41_6880 / 0x10C | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:591 | EMPTY | empty body; fuzzy 1.5% | spawned; module lines 44.6%, up2 45.1% |
| 33 | MediumIng | `CMediumIng::FollowAttackPattern(CStateManager&, EStateMsg, float)` | fn_41_71BC / 0x108 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:573 | EMPTY | empty body; fuzzy 1.5% | spawned; module lines 44.6%, up2 45.1% |
| 34 | MediumIng | `CMediumIng::FollowApproachPattern(CStateManager&, EStateMsg, float)` | fn_41_70B4 / 0x108 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:574 | EMPTY | empty body; fuzzy 1.5% | spawned; module lines 44.6%, up2 45.1% |
| 35 | MediumIng | `CMediumIng::SetMeshPathDestination(CStateManager&, float)` | fn_41_2A74 / 0x100 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:593 | EMPTY | empty body; fuzzy 1.6% | spawned; module lines 44.6%, up2 45.1% |
| 36 | MediumIng | `CMediumIng::SubStart(CStateManager&, EStateMsg, float)` | fn_41_7394 / 0xE4 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:571 | EMPTY | empty body; fuzzy 1.8% | spawned; module lines 44.6%, up2 45.1% |
| 37 | MediumIng | `fn_41_49CC` | fn_41_49CC / 0x1C0 | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |
| 38 | MediumIng | `fn_41_6F18` | fn_41_6F18 / 0x19C | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |
| 39 | MediumIng | `CMediumIng::PathFind(CStateManager&, EStateMsg, float)` | fn_41_72C4 / 0xD0 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:572 | EMPTY | empty body; fuzzy 1.9% | spawned; module lines 44.6%, up2 45.1% |
| 40 | MediumIng | `CMediumIng::SetRetreatDestination(CStateManager&, float)` | fn_41_2564 / 0xB8 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:592 | EMPTY | empty body; fuzzy 2.2% | spawned; module lines 44.6%, up2 45.1% |
| 41 | MediumIng | `CMediumIngTentacle::UpdateShake(CStateManager&, float)` | fn_41_9ED0 / 0xB4 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:616 | EMPTY | empty body; fuzzy 2.2% | spawned; module lines 44.6%, up2 45.1% |
| 42 | MediumIng | `CMediumIng::Render(const CStateManager&) const` | fn_41_74F8 / 0xAC | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:599 | EMPTY | empty body; fuzzy 2.3% | spawned; module lines 44.6%, up2 45.1% |
| 43 | MediumIng | `CMediumIngTentacle::GetTouchBounds() const` | fn_41_93FC / 0xA8 | 0xC | src/MetroidPrime/Enemies/CMediumIng.cpp:609 | TINY | compiled 0xC of 0xA8 (7%); fuzzy 7.0% | spawned; module lines 44.6%, up2 45.1% |
| 44 | MediumIng | `fn_41_4470` | fn_41_4470 / 0x128 | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |
| 45 | MediumIng | `CMediumIng::Death(CStateManager&, const CVector3f&, EScriptObjectState)` | fn_41_1B18 / 0x90 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:601 | EMPTY | empty body; fuzzy 2.8% | spawned; module lines 44.6%, up2 45.1% |
| 46 | MediumIng | `fn_41_6BFC` | fn_41_6BFC / 0x10C | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |
| 47 | MediumIng | `CMediumIng::PreRender(CStateManager&)` | fn_41_75A4 / 0x78 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:598 | EMPTY | empty body; fuzzy 3.3% | spawned; module lines 44.6%, up2 45.1% |
| 48 | MediumIng | `fn_41_4308` | fn_41_4308 / 0xE0 | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |
| 49 | DOL:MetroidPrime | `CDamageInfo::WithNoImmunity() const` | fn_800B5F8C / 0x64 | 0x0 | src/MetroidPrime/CDamageInfo.cpp | STUB | no upstream definition; PortLinkStubs reach stub | DOL (resident) |
| 50 | DOL:Weapons | `CBloggProjectile::ApplyDamageToOneActor(CStateManager&, const CDamageInfo&, TUniqueId, const CVector3f&)` | fn_80213508 / 0x68 | 0x4 | src/MetroidPrime/Weapons/CBloggProjectile.cpp:15 | EMPTY | empty body; fuzzy 3.8% | DOL (resident) |
| 51 | DOL:Animation | `CVirtualBone::BuildSkinningMatrices(const CTransform4f&, SSkinningMatrices&, bool) const` | fn_80310838 / 0xB8 | 0xE8 | src/Kyoto/Animation/DolphinCVirtualBone.cpp:168 | LOWMATCH | compiled 0xE8; fuzzy 5.7% | DOL (resident) |
| 52 | MediumIng | `CMediumIng::WaitForLocomotion(CStateManager&, EStateMsg, float)` | fn_41_1E78 / 0x58 | 0x4 | src/MetroidPrime/Enemies/CMediumIng.cpp:583 | EMPTY | empty body; fuzzy 4.5% | spawned; module lines 44.6%, up2 45.1% |
| 53 | DOL:MetroidPrime | `LdrToDamageInfo(const SLdrDamageInfo&)` | fn_8023B2F8 / 0xA4 | 0xCC | src/MetroidPrime/ScriptLoader.cpp:288 | LOWMATCH | compiled 0xCC; fuzzy 0.9% | DOL (resident) |
| 54 | MediumIng | `fn_41_4598` | fn_41_4598 / 0x90 | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |
| 55 | IngSnatchingSwarm | `fn_33_4F44` | fn_33_4F44 / 0x5C | 0x0 | src/MetroidPrime/Enemies/CIngSnatchingSwarm.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | not spawned; module lines 0.2% |
| 56 | MediumIng | `fn_41_9C8C` | fn_41_9C8C / 0x54 | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |
| 57 | MediumIng | `fn_41_3064` | fn_41_3064 / 0x48 | 0x0 | src/MetroidPrime/Enemies/CMediumIng.cpp | UNPAIRED | unnamed retail fn; no upstream symbol of its size | spawned; module lines 44.6%, up2 45.1% |

Hollow on the GameCube side but with a TARGET_PC body (the port's own; not a hole): DOL:MetroidPrime fn_8002B128 src/MetroidPrime/CAnimData.cpp:303, DOL:Player fn_800C406C src/MetroidPrime/Player/CMorphBall.cpp:1406
