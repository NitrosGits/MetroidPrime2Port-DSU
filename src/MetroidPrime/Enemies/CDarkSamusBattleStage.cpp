#include "MetroidPrime/Enemies/CDarkSamusBattleStage.hpp"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

// @ fn_11_BE8
CDarkSamusBattleStage::CDarkSamusBattleStage(TUniqueId uid, const rstl::string& name,
                                             const CEntityInfo& info,
                                             const SLdrDSStageInfo& stageInfo)
: CEntity(uid, info, name, 0), mStageInfo(stageInfo) {}

CDarkSamusBattleStage::~CDarkSamusBattleStage() {}

// @ fn_11_B70
const SLdrDSStageInfo& CDarkSamusBattleStage::GetStageInfo() const { return mStageInfo; }

CEntity* LoadDarkSamusBattleStage(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrDarkSamusBattleStage sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrDarkSamusBattleStage.inc"

  return rs_new CDarkSamusBattleStage(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                      LdrToEntityInfo(info, sldrThis.editorProperties),
                                      sldrThis.stage);
}

#ifndef MONOLITHIC
// @ fn_11_44
static void SetFuncPtrs() {
  static SScriptDarkSamusBattleStage_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadDarkSamusBattleStage;
  SetSScriptDarkSamusBattleStage_FuncPtrs(&funcPtrs);
}

extern "C" void RELMain() { SetFuncPtrs(); }

extern "C" void RELExit() { SetSScriptDarkSamusBattleStage_FuncPtrs(nullptr); }
#endif
