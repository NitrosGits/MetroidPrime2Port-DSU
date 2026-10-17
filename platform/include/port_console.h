#pragma once

class CStateManager;

// The debug command console: with `MP2_CONSOLE=<port>` (1 = 4780) the port listens on
// 127.0.0.1 and runs one command per line, e.g. `status`, `warp 7`, `door`. Every reply ends with a
// line `=> ok` or `=> err: <why>`. tools/mp2con.py is the client, and
// `printf 'status\n' | nc 127.0.0.1 4780` is the shortest way in. Objects and players:
// `objs [filter]`, `obj <id>`, `send <id> <msg>`, `visor`, `beam`, `tp <x> <y> <z>`, `items`.
//
// Commands that read the world run inside the state manager's update, where every object pointer is
// live (`Tick`); nothing here touches the game from the network thread. It is modelled on
// MetroidPrimePort's `port_console.cpp` (same protocol, same one-client-at-a-time shape), with the
// commands this tree can actually serve.
//
// In every build; it does nothing unless MP2_CONSOLE is set.

namespace PortConsole {

// Reads MP2_CONSOLE and starts the listener. Called once from the platform's start-up.
void Start();
bool Enabled();
// Runs one queued command, if any, on the game thread. `CStateManager::PortWarpTick` calls it every
// update.
void Tick(CStateManager& mgr);

} // namespace PortConsole
