#include "MetroidPrime/CRuleSet.hpp"
#include "Kyoto/CFactoryMgr.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "Kyoto/Streams/CInputStream.hpp"

CFactoryFnReturn FRuleSetFactory(const SObjectTag& tag, CInputStream& in,
                                 const CVParamTransfer& xfer) {
  return rs_new CRuleSet(in);
}

CRuleSet::CRuleSet(CInputStream& input) {
#ifdef TARGET_PC
  // The port can supply an empty/short stream for a missing or failed resource.
  // Retail assumes its 4-byte magic, 1-byte version, 4-byte parent and 2-byte count exist.
  const unsigned long pos = input.GetReadPosition();
  if (pos > input.GetBufferSize() || input.GetBufferSize() - pos < 11) {
    return;
  }
#endif
  input.ReadInt32(); // magic
  input.ReadInt8();  // version
  CAssetId parentRule = static_cast< CAssetId >(input.ReadInt32());
#ifdef TARGET_PC
  // Missing resources also arrive as zero-filled buffers. Loading their parent 0
  // re-enters this factory before the first RULE 0 has finished loading, until OOM.
  // Retail only skips parent -1 (0x801F6540 addis / 0x801F6544 cmplwi / 0x801F6548 beq);
  // no shipped RULE has parent 0. Treat this invalid resource as no rules.
  if (parentRule == 0) {
    return;
  }
#endif
  if (parentRule != kInvalidAssetId) {
    mParentRule = TLockedToken< CRuleSet >(gpSimplePool->GetObj(SObjectTag('RULE', parentRule)));
  }
  int ruleCount = input.ReadInt16();
  mRules.reserve(ruleCount);
  for (int i = 0; i < ruleCount; ++i) {
    mRules.push_back_unsafe(CRuleSetRule(input));
  }
}

CRuleSetRule::CRuleSetRule(CInputStream& input) {
  int conditionCount = input.ReadInt16();
  mConditions.reserve(conditionCount);
  for (int i = 0; i < conditionCount; ++i) {
    mConditions.push_back_unsafe(CRuleCondition(input));
  }

  int actionCount = input.ReadInt16();
  mActions.reserve(actionCount);
  for (int i = 0; i < actionCount; ++i) {
    mActions.push_back_unsafe(CRuleAction(input));
  }
}

CRuleCondition::CRuleCondition(CInputStream& input)
: mId(input.ReadInt32()), mOperator(input.ReadInt8()), mValue(input.ReadInt8(), input) {}

CRuleAction::CRuleAction(CInputStream& input) : mId(input.ReadInt32()) {
  int propCount = input.ReadInt8();
  mProperties.reserve(propCount);
  for (int i = 0; i < propCount; ++i) {
    mProperties.push_back_unsafe(CRuleValue(3, input));
  }
}

CRuleValue::CRuleValue(int type, CInputStream& input) : mType(type), mValue(input.ReadInt32()) {}

bool CRuleValue::GetBool() const { return *reinterpret_cast< const bool* >(&mValue); }

int CRuleValue::GetInt() const { return mValue; }

float CRuleValue::GetFloat() const { return *reinterpret_cast< const float* >(&mValue); }
