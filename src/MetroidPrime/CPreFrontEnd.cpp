#include "MetroidPrime/CPreFrontEnd.hpp"

#include "MetroidPrime/CArchitectureMessage.hpp"
#include "MetroidPrime/CMain.hpp"
#include "MetroidPrime/CMemoryCard.hpp"

#include "Kyoto/CResFactory.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"

#ifdef TARGET_PC
#include "MetroidPrime/PortSkipFrontEnd.hpp"
#endif

CPreFrontEnd::CPreFrontEnd() : CIOWin(rstl::string_l("Pre front-end window")) {}

CIOWin::EMessageReturn CPreFrontEnd::OnMessage(const CArchitectureMessage& msg,
                                               CArchitectureQueue&) {
  if (msg.GetType() == kAM_TimerTick) {
    if (!gpResourceFactory->GetResLoader().AreAllPaksLoaded()) {
      return kMR_Exit;
    }

    gpResourceFactory->AsyncIdle(1000000, false);

    gpMain->MemoryCardInitializePump();
    if (!gpMemoryCard) {
      return kMR_Exit;
    }

#ifdef TARGET_PC
    // MP2_SKIP_FRONTEND=1: the requested save has to be on the card and in the game state *before*
    // the flow leaves this window, because the next state (kCFS_FrontEnd) is what starts the game
    // from it. This holds the flow here while the card read is still coming in, and returns true
    // (nothing to wait for) when the flag is unset or the skip gave up. See PortSkipFrontEnd.cpp.
    if (!PortSkipFrontEnd::Pump()) {
      return kMR_Exit;
    }
#endif

    return kMR_RemoveIOWinAndExit;
  } else {
    return kMR_Normal;
  }
}

void CPreFrontEnd::Draw() const { CGraphics::SetIsBeginSceneClearFb(true); }

CPreFrontEnd::~CPreFrontEnd() {}
