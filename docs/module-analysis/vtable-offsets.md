# Vtable slot lookup (CEntity → CActor → CPhysicsActor → CAi → CPatterned)

GC/Metrowerks vtables start with two leading words — 0x00 RTTI (null in the REL data), 0x04 zero — so the
first virtual sits at byte offset 0x8 and each slot is 4 bytes. Slots are assigned in declaration order
down the inheritance chain, base class first; an override reuses its base's slot and new virtuals append.
So in m2c pseudocode `(*obj->unk0)->unkXX(obj, ...)`, XX is the byte offset in the table below (e.g.
`unk52` = 0x14 = `Think`). Verified against the SpacePirate vtable `lbl_72_data_8D8`
(`../MetroidPrime2Port/build/G2ME01/SpacePirate/asm/auto_04_00000000_data.s`): 82 words = 80 slots,
every entry matching the header-derived order. The last column shows what a call through that slot
resolves to in SpacePirate's vtable: `fn_72_X` = SpacePirate's own override, otherwise the inherited
base implementation (demangled). Only the non-const `GetHealthInfo`/`GetSearchPath`/`GetPointSearchPath`
are virtual; their const overloads are inline non-virtual wrappers and take no slot.

| offset | class that declares it | method signature | SpacePirate override fn |
|---|---|---|---|
| 0x08 | CEntity | `~CEntity()` (slot holds the deleting dtor) | `fn_72_F10C` → body `fn_72_F1CC` |
| 0x0C | CEntity | `CEntity* TypesMatch(int typeId) const` | `fn_72_F49C` path; slot = `TypesMatch__12CSpacePirateCFi` |
| 0x10 | CEntity | `void PreThink(float dt, CStateManager& mgr)` | `fn_72_24C4` |
| 0x14 | CEntity | `void Think(float dt, CStateManager& mgr)` | `fn_72_1980` |
| 0x18 | CEntity | `void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg&)` | `fn_72_E498` |
| 0x1C | CEntity | `void SetActive(bool active)` | inherited `CActor::SetActive` |
| 0x20 | CActor | `void ClearFluidList(CStateManager& mgr)` | inherited `CActor::ClearFluidList` |
| 0x24 | CActor | `void PreRender(CStateManager&)` | `fn_72_38E4` |
| 0x28 | CActor | `void AddToRenderer(const CStateManager&) const` | inherited `CPatterned::AddToRenderer` |
| 0x2C | CActor | `void Render(const CStateManager&) const` | `fn_72_37C4` |
| 0x30 | CActor | `bool CanRenderUnsorted(const CStateManager&) const` | inherited `CPatterned::CanRenderUnsorted` |
| 0x34 | CActor | `void PreRenderAllViewports(CStateManager& mgr)` | `fn_72_3A0C` |
| 0x38 | CActor | `CHealthInfo* HealthInfo()` | inherited `CAi::HealthInfo` |
| 0x3C | CActor | `const CHealthInfo* GetHealthInfo() const` | `fn_72_64` (forwards to slot 0x38) |
| 0x40 | CActor | `const CDamageVulnerability* GetDamageVulnerability() const` | `fn_72_3AB4` |
| 0x44 | CActor | `const CDamageVulnerability* GetDamageVulnerability(const CVector3f&, const CVector3f&, const CDamageInfo&) const` | `fn_72_3A88` |
| 0x48 | CActor | `rstl::optional_object<CAABox> GetTouchBounds() const` | `fn_72_17C8` |
| 0x4C | CActor | `void Touch(CActor&, CStateManager&)` | `fn_72_E3D8` |
| 0x50 | CActor | `CVector3f GetOrbitPosition(const CStateManager&) const` | inherited `CPatterned::GetOrbitPosition` |
| 0x54 | CActor | `CVector3f GetAimPosition(const CStateManager&, float) const` | inherited `CPatterned::GetAimPosition` |
| 0x58 | CActor | `CVector3f GetHomingPosition(const CStateManager&, float) const` | inherited `CActor::GetHomingPosition` |
| 0x5C | CActor | `CVector3f GetScanObjectIndicatorPosition(const CStateManager&) const` | inherited `CActor::GetScanObjectIndicatorPosition` |
| 0x60 | CActor | `EWeaponCollisionResponseTypes GetCollisionResponseType(const CVector3f&, const CVector3f&, const CWeaponMode&, int) const` | inherited `CPatterned::GetCollisionResponseType` |
| 0x64 | CActor | `void FluidFXThink(EFluidState, CScriptWater&, CStateManager&)` | inherited `CAi::FluidFXThink` |
| 0x68 | CActor | `void OnScanStateChange(EScanState, CStateManager&)` | inherited `CActor::OnScanStateChange` |
| 0x6C | CActor | `CAABox GetSortingBounds(const CStateManager&) const` | `fn_72_35CC` |
| 0x70 | CActor | `void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type, float dt)` | `fn_72_3B28` (jump table `data_A20`, falls back to `CPatterned::DoUserAnimEvent`) |
| 0x74 | CActor | `CScannableObjectInfo* GetScannableObjectInfo() const` | inherited `CPatterned::GetScannableObjectInfo` |
| 0x78 | CActor | `void ProcessSoundEvent(int sfxId, float weight, int flags, float fallOff, float maxDist, const CSegId&, ushort, ushort, float, uchar, uchar, float, const CVector3f&, int, CStateManager&, bool)` | inherited `CActor::ProcessSoundEvent` |
| 0x7C | CPhysicsActor | `const CCollisionPrimitive* GetCollisionPrimitive() const` | inherited `CPhysicsActor::GetCollisionPrimitive` |
| 0x80 | CPhysicsActor | `CTransform4f GetPrimitiveTransform() const` | inherited `CPhysicsActor::GetPrimitiveTransform` |
| 0x84 | CPhysicsActor | `void CollidedWith(const TUniqueId&, const CCollisionInfoList&, CStateManager&)` | inherited `CPatterned::CollidedWith` |
| 0x88 | CPhysicsActor | `float GetStepDownHeight() const` | inherited `CPhysicsActor::GetStepDownHeight` |
| 0x8C | CPhysicsActor | `float GetStepUpHeight() const` | inherited `CPhysicsActor::GetStepUpHeight` |
| 0x90 | CPhysicsActor | `bool IsOnStaticGround() const` | inherited `CPatterned::IsOnStaticGround` |
| 0x94 | CPhysicsActor | `float GetWeight() const` | inherited `CPhysicsActor::GetWeight` |
| 0x98 | CAi | `void Death(CStateManager&, const CVector3f& direction, EScriptObjectState state) = 0` | `fn_72_C538` |
| 0x9C | CAi | `void KnockBack(CStateManager&, const CKnockBackInfo&) = 0` | `fn_72_C684` |
| 0xA0 | CAi | `CDamageVulnerability* DamageVulnerability()` | inherited `CAi::DamageVulnerability` |
| 0xA4 | CAi | `void TakeDamage(const CVector3f& direction, float magnitude)` | `fn_72_30` |
| 0xA8 | CAi | `bool CanBeShot(const CStateManager&, int)` | `fn_72_5C` (returns true) |
| 0xAC | CAi | `bool IsListening() const` | `fn_72_10` (returns true) |
| 0xB0 | CAi | `bool Listen(CStateManager&, const CVector3f& position, EListenNoiseType type)` | `fn_72_E318` |
| 0xB4 | CAi | `CVector3f GetOrigin(const CStateManager&, const CTeamAiRole&, const CVector3f&) const` | `fn_72_E090` |
| 0xB8 | CPatterned | `void RenderSystemsToBeDrawnFirst(const CStateManager&, uint mask, uint target) const` | inherited `CPatterned::RenderSystemsToBeDrawnFirst` |
| 0xBC | CPatterned | `void RenderSystemsToBeDrawnLast(const CStateManager&, uint mask, uint target) const` | inherited `CPatterned::RenderSystemsToBeDrawnLast` |
| 0xC0 | CPatterned | `void Freeze(CStateManager&, const CVector3f& position, CUnitVector3f direction, float duration, float intoFreezeDuration)` | inherited `CPatterned::Freeze` |
| 0xC4 | CPatterned | `void ThinkAboutMove(float dt)` | inherited `CPatterned::ThinkAboutMove` |
| 0xC8 | CPatterned | `uchar GetModelAlphau8(const CStateManager&) const` | `fn_72_3AE8` |
| 0xCC | CPatterned | `void Burn(CStateManager&, float duration, float damage)` | inherited `CPatterned::Burn` |
| 0xD0 | CPatterned | `void Shock(CStateManager&, float duration, float damage)` | inherited `CPatterned::Shock` |
| 0xD4 | CPatterned | `void MassiveDeath(CStateManager&)` | inherited `CPatterned::MassiveDeath` |
| 0xD8 | CPatterned | `void MassiveFrozenDeath(CStateManager&)` | inherited `CPatterned::MassiveFrozenDeath` |
| 0xDC | CPatterned | `CProjectileInfo* ProjectileInfo()` | `fn_72_4F0C` |
| 0xE0 | CPatterned | `CPathFindSearch* GetSearchPath()` | `fn_72_8` (returns this+0x920) |
| 0xE4 | CPatterned | `CPathFindPointSearch* GetPointSearchPath()` | `fn_72_40` (returns null) |
| 0xE8 | CPatterned | `CDamageInfo GetContactDamage() const` | inherited `CPatterned::GetContactDamage` |
| 0xEC | CPatterned | `void UpdateHitDamageTime(float dt)` | inherited `CPatterned::UpdateHitDamageTime` |
| 0xF0 | CPatterned | `void SetupStateMachine(CStateManager& mgr)` | `fn_72_F0A0` |
| 0xF4 | CPatterned | `CRagDoll* GetRagDoll() const` | `fn_72_17A0` |
| 0xF8 | CPatterned | `bool CanBeIngPossessed(CStateManager&) const` | inherited `CPatterned::CanBeIngPossessed` |
| 0xFC | CPatterned | `bool CanBeUnPossessed(CStateManager&) const` | inherited `CPatterned::CanBeUnPossessed` |
| 0x100 | CPatterned | `void SetIngPossessed(bool possessed, CStateManager&)` | inherited `CPatterned::SetIngPossessed` |
| 0x104 | CPatterned | `void SetIngPossessed(bool possessed, float duration, CStateManager&)` | inherited `CPatterned::SetIngPossessed` |
| 0x108 | CPatterned | `void SetAttackTarget(CStateManager&, TUniqueId target)` | `fn_72_E120` |
| 0x10C | CPatterned | `TUniqueId GetAttackTarget() const` | `fn_72_18` (u16 at this+0xa94) |
| 0x110 | CPatterned | `bool IsOnGround() const` | `fn_72_48` (bit in this+0x34c) |
| 0x114 | CPatterned | `float GetGravityConstant() const` | `fn_72_24` (returns 50.0) |
| 0x118 | CPatterned | `bool IsScanVisorSelfRender() const` | `fn_72_0` (returns true) |
| 0x11C | CPatterned | `CAABox GetScanVisorRenderBounds(const CStateManager&) const` | `fn_72_343C` |
| 0x120 | CPatterned | `void ScanVisorRender(const CStateManager&, const CTransform4f&, const CModelFlags&) const` | `fn_72_339C` |
| 0x124 | CPatterned | `const rstl::optional_object<TCachedToken<CGenDescription>>& GetDeathExplosionParticle() const` | `fn_72_54` (this+0x754, base member) |
| 0x128 | CPatterned | `float GetDeathTimeScale() const` | inherited `CPatterned::GetDeathTimeScale` |
| 0x12C | CPatterned | `bool TryToBeCaptured(CStateManager&)` | `fn_72_18BC` |
| 0x130 | CPatterned | `void IssueDeathBodyCommand(CStateManager&, const CVector3f& direction)` | inherited `CPatterned::IssueDeathBodyCommand` |
| 0x134 | CPatterned | `float GetFadeOnDeathTime() const` | inherited `CPatterned::GetFadeOnDeathTime` |
| 0x138 | CPatterned | `CVector3f GetIngSnatchingNormal(float t) const` | inherited `CPatterned::GetIngSnatchingNormal` |
| 0x13C | CPatterned | `CVector3f GetIngSnatchingPoint(float t) const` | inherited `CPatterned::GetIngSnatchingPoint` |
| 0x140 | CPatterned | `float GetIngSnatchingModelOverlapSize() const` | inherited `CPatterned::GetIngSnatchingModelOverlapSize` |
| 0x144 | CPatterned | `void RenderIngSnatchingTransition(const CStateManager&) const` | inherited `CPatterned::RenderIngSnatchingTransition` |

## Cross-check against SpacePirate.md §1

All 36 overrides listed there land on the slot with the matching name: CEntity dtor/PreThink/Think/
AcceptScriptMsg → 0x08/0x10/0x14/0x18; CActor PreRender/Render/PreRenderAllViewports/GetHealthInfo/
GetDamageVulnerability ×2/GetTouchBounds/Touch/GetSortingBounds/DoUserAnimEvent → 0x24/0x2C/0x34/0x3C/
0x40/0x44/0x48/0x4C/0x6C/0x70; CAi Death/KnockBack/TakeDamage/CanBeShot/IsListening/Listen/GetOrigin →
0x98/0x9C/0xA4/0xA8/0xAC/0xB0/0xB4; CPatterned GetModelAlphau8/ProjectileInfo/GetSearchPath/
GetPointSearchPath/SetupStateMachine/GetRagDoll/SetAttackTarget/GetAttackTarget/IsOnGround/
GetGravityConstant/IsScanVisorSelfRender/GetScanVisorRenderBounds/ScanVisorRender/
GetDeathExplosionParticle/TryToBeCaptured → 0xC8/0xDC/0xE0/0xE4/0xF0/0xF4/0x108/0x10C/0x110/0x114/
0x118/0x11C/0x120/0x124/0x12C. No mismatches. (The parenthesized hex in that section — 0x6E0 for Think,
0x944 for AcceptScriptMsg, 0x100 for GetSortingBounds, … — are the functions' *sizes* from
`config/G2ME01/rels/SpacePirate/symbols.txt`, not vtable offsets.) The same layout holds for every class
in the chain, so the table names slots for all Patterned-family enemies, not just SpacePirate.
