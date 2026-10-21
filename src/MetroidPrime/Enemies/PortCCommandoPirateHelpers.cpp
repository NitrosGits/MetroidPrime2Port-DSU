// The stateless helpers of the CommandoPirate REL (module 9) - the bodies that are not the state
// machine's states or triggers (the CommandoPirate lane owns those; the FSM2 tables
// lbl_9_data_198 and lbl_9_data_568 name them). The analysis is
// docs/module-analysis/CommandoPirate.md, the per-function table
// docs/module-analysis/CommandoPirate_functions.tsv.
//
// Every body here is retail's, with the asm offsets in the comments. These are the module's own
// copies of the base class's virtuals, so the port keeps them as the class's own members.
#include "MetroidPrime/Enemies/CCommandoPirate.hpp"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "Kyoto/Math/CVector3f.hpp"

#include <math.h>

// @ fn_9_BE4C
// vtable slot 0xb0 - what the actor hears. Retail's body (0x0000BE4C..0x0000BF3C):
//   - the base's flag byte at guest this+0x420: without its 0x40 bit (the host's mAlive, the
//     "the actor is alive" flag) the actor hears nothing (0x0000BE4C..0x0000BE60);
//   - the noise type: 4 (a path noise) and 0 (a shot) are the two that matter, every other type
//     returns 0 (0x0000BE64..0x0000BE78);
//   - type 4: the squared distance to the noise against the constant 1600.0f (rodata_4E4); inside
//     it the 0x80 bit of the module's byte at guest this+0xc63 is set (0x0000BE7C..0x0000BEC4);
//   - type 0: the squared distance against the hearing radius (the data block's field at guest
//     this+0x81c), then the base's height range at guest this+0x3d4 against the constant 0.0f
//     (rodata_494): the height range wins the tie, and otherwise the noise's own height distance
//     decides (0x0000BEC8..0x0000BF2C);
//   - the constant 0.0f is written to the module's float at guest this+0xb60 when the actor
//     heard the shot (0x0000BF2C..0x0000BF38).
bool CCommandoPirate::xBE4C_Listen(CStateManager& mgr, const CVector3f& position,
                                  EListenNoiseType type) {
  if (!GetAlive()) {                                        // 0x0000BE4C..0x0000BE60
    return false;
  }
  const CVector3f delta = position - GetTranslation();      // guest this+0x54..0x5c
  // The squared distance, as retail sums it (the three squares, 0x0000BE88..0x0000BEB4).
  const float distSq = delta.GetX() * delta.GetX() + delta.GetY() * delta.GetY() +
                       delta.GetZ() * delta.GetZ();
  if (type == 4) {                                         // a path noise, 0x0000BE74
    if (distSq >= 1600.f) {                                 // rodata_4E4, 0x0000BE7C..0x0000BEC4
      return false;
    }
    xc63_ |= 0x80;                                         // the byte at guest this+0xc63
    return true;
  }
  if (type == 0) {                                         // a shot, 0x0000BEC8
    const float radius = mData.mHearingRadius;             // guest this+0x81c
    if (distSq >= radius * radius) {                        // 0x0000BEF4..0x0000BF14
      return false;
    }
    const float heightRange = mDetectionHeightRange;       // guest this+0x3d4
    if (heightRange != 0.f && delta.GetZ() * delta.GetZ() >= heightRange * heightRange) {
      return false;                                        // 0x0000BF20..0x0000BF38
    }
    xb60_ = 0.f;                                           // guest this+0xb60 = rodata_494 (0.0f)
    return true;
  }
  return false;                                            // 0x0000BF3C
}
