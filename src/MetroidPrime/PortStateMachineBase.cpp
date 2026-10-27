// Port-only: TStateMachineStateBase<T>'s destructor for the owners the port instantiates.
// Retail defines these explicit specializations in the main DOL (see CIngPuddle.cpp and
// CIngSnatchingSwarm.cpp); the base holds no members, so each is empty. They replace the
// PortLinkStubs.cpp reach stubs, which logged a reach-stub whenever an FSM state was destroyed
// (Splitter's generated spawns in 09_cliff_temple).

#include "MetroidPrime/TStateMachineState.hpp"

#ifndef _MSC_VER
// MSVC: the units using these instantiate the header's template definition as COMDATs, which
// lld-link will not let an explicit specialization sit beside (ELF linkers take the strong one).
class CPatterned;
class CPlayerGun;
class CGrappleArm;
class CPlayerBodyController;

template <>
TStateMachineStateBase< CPatterned >::~TStateMachineStateBase() {}
template <>
TStateMachineStateBase< CPlayerGun >::~TStateMachineStateBase() {}
template <>
TStateMachineStateBase< CGrappleArm >::~TStateMachineStateBase() {}
template <>
TStateMachineStateBase< CPlayerBodyController >::~TStateMachineStateBase() {}
#endif
