// Port: the module entry points for a REL that has no host body yet.
//
// `tools/modules_port.txt` registers each of these modules against this one file, and
// tools/scope_module.py gives every module its own copy of the pair (RELMain becomes
// mp_rel_<Name>_prolog), so twenty modules can share it without colliding at link time.
//
// What this buys: an area whose module list names such a module used to stop the whole game
// ("module prolog: RELProd/X.rel is not compiled into the port"), which made most of the map
// unreachable. With the entry point present the area loads - the module's enemies simply never
// appear, because nothing registers their loaders (the DOL's script loader has no entry for a
// type whose module is a stub, and the `S<Module>_FuncPtrs` proxies are absent for these). The
// print below is what makes that visible in a run's log: it fires when an area asks for a module
// whose code is still missing.
//
// Replacing a stub with the real thing is the pattern used for WallCrawler
// (src/MetroidPrime/Enemies/PortCWallCrawler.cpp): point the module at a file with the class's
// implementation, its loader(s) and the entry points, and drop this file from the module's line
// in tools/modules_port.txt.

#include "types.h"

#include <cstdio>

void RELMain() {
  std::fprintf(stderr,
               "[modules] a module without a host body was linked in: its actors will not "
               "appear (see src/MetroidPrime/PortModuleStubs.cpp)\n");
}

void RELExit() {}
