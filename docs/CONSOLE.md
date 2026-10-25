# Debug console

`MP2_CONSOLE=<port>` opens a TCP console on that port; `tools/drivers/con.py <port> <cmdfile>` drives
it (see `docs/RUNNING.md`). Each line is one command, `a; b; c` runs several in one game tick, and
every command answers `=> ok` or `=> err: <why>`. `help` prints the list below, taken from the
dispatch table `kCommands` in `platform/port_console.cpp`; `tests/port_console_docs.py` (ctest
`port_console_docs`) fails when a command in that table is missing here, so a new command needs
a line in this file.

`status --json` prints one line of JSON (frame, world, area, mrea, position, yaw, speed, morph,
hp, hp_max, visor, beam, freecam) for scripts.

## Commands
- `status [--json]` — world, area, player position, health, visor, beam, morph state; --json: one line for scripts
- `areas` — the current world's area slots (index, MREA id, name if loaded)
- `warp <slot|id|name>` — load an area of the current world (what MP2_TELEPORT takes)
- `nextarea | prevarea` — the next or previous existing area of this world (also `prevarea`)
- `door | dock | station` — stand in front of the nearest door / doorway / save station (also `dock`, `station`)
- `heal | kill [id]` — refill energy; kill the player (a respawn test) or an actor (also `kill`)
- `damage <id> <n> [kb] [w]` — hit an actor for n as a player shot (w = power, missile, cannonball, dark, light, annihilator, powerbomb, bomb, boostball, screwattack, darkburst, sunburst, sonicboom, supermissile; kb = knockback power)
- `layer <n>` — activate a layer of the current area
- `god <0|1>` — god mode (the F1 cheat)
- `mpforce <0|1>` — IsMultiplayer reports true (multiplayer-only arms)
- `overlay <page>[/<sub>]|off` — open the F1 window on a page (name prefixes: `overlay video/qual`)
- `gpuselftest` — run the GPU self-test next frame; PASS/FAIL lines go to the log
- `state save|load <slot>` — the F5/F9 save states (load 0 = undo the last load)
- `resume [status|save|load|clear]` — quick resume (savestates/resume.mp2ss): show, snapshot now, load, delete
- `skipcine <0|1>` — skip cutscenes the player has not seen (the F1 setting)
- `gunoffset [<r> <f> <u>]` — arm cannon offset in metres (+-1), as the F1 Render row does
- `msaa <1|4> | aniso <1..16>` — anti-aliasing and anisotropic filtering (applied next frame) (also `aniso`)
- `hudscale <50..100>` — HUD scale in percent, as the F1 HUD row does
- `crosshair [<25..100>]` — aim crosshair size, `style classic|remastered`, opacity, color <r> <g> <b>|clear, aim <0|1> (F1 HUD)
- `uiscale <50..200>` — the F1 window's size in percent, as the Render row does
- `vsync <0|1>` — vertical sync, as the F1 Render row does
- `interp [0|1]` — smooth frames: the camera between ticks with the cap off
- `aspect [4:3|16:9|window]` — aspect ratio, as the F1 Video > Display row does
- `boost` — boost-ball release on the morphed player
- `noise <type> [x y z]` — InformListeners at the player (6 = safe-zone noise)
- `gameevent <ev> [src] [tgt] [value] [as <proxyType>]` — deliver a game event to every CScriptPlayerProxy listener (ev: score kill generic scan damage stop spawn; value: float for damage, state for generic)
- `fastmorph <0|1>` — fast morph: the morph transition takes 0.2 s, not a second
- `walkspeed [50..200]` — walk speed in percent (shows the value with no argument); `walk_speed` in SETTINGS.md
- `pointer <x> <y> move|click|dclick|tap|dtap|down|up [l|r|touch]|drag <x1> <y1> [l|r|touch]|wheel <n>|pinch <ratio>`, `pointer back|where|hits` — scripted mouse/touch input for the pointer menus, in GUI (EFB) pixels as screenshots show them; `where` prints the position and the widget rects under it, `hits` lists the recorded rects
- `hintshow` — show the hint the game would show next (F1 > Game > Show current hint)
- `springball <0|1>` — spring ball: C-stick up in Morph Ball bomb-jumps (needs Bombs)
- `lockon <0|1>` — toggle lock-on: an L press latches until the next press
- `sticky <0|1>` — sticky charge: a long A hold keeps charging after the release
- `pointeraim [on|off|<x> <y>] pointer aim (the F1 row), or place its pointer (-1..1)`
- `save [info] [<file>]` — the card's save slots, or one .gci on disk: world, items, time
- `cardbackup <0|1>` — back the card up before each write (the F1 Saves row)
- `settings [<key> <value>|self-test]` — settings.cfg's entries; set one (and save), or check every key round-trips
- `logfile <0|1>` — stdout and stderr also go to metroid_prime2_port.log (next launch)
- `objs [filter]` — objects of the world: uid, editor id, class, area, position
- `obj <id>` — one object (an id as `objs` prints it: u046e, or e<editor id>): health, fsm, messages
- `send <id> <msg> [from <id>] deliver a script message to an object (name or number), from a sender`
- `covdump` — coverage build: write the gcov counters now (and zero them)
- `hurt <id> <amount> [weapon] damage an object as a shot would (weapon 0 = power beam)`
- `frame` — the game updates run so far (what MP2_FRAMES counts)
- `fsm <id> <state>` — force a patterned actor's AI state by name (as `obj` prints it; FSM2 sub-machines too; an unknown name lists the valid ones)
- `bodystate <id> <n>` — set a patterned actor's body state (the pas::EAnimationState index)
- `statecall <id> <state> <msg> [dt]` — run one FSM2 state body once (msg 0/1/2), also states with no FSM node
- `codecall <id> <code> [dt]` — run one FSM2 code function once (codes run on transitions)
- `pathfail <id> on|off` — the actor's path search fails (Search, FindClosestReachablePoint)
- `setarea <id> <area>` — set an entity's current area id (put it back after; obj shows it; loaded areas only)
- `var <id> <name> [value]` — a module field a port registered (dghd.attack, dghd.flags, dghd.sweeptime); var player spawnmorph [n]
- `scaninfo <id>` — the SCAN asset an actor's GetScannableObjectInfo answers
- `orbit [id]` — lock-on state; with id: orbit position, ValidateOrbitTargetId
- `hint <id> [type] [area n]` — an AI hint's type, area, active flag, radius; set type / area (coverage)
- `place <id> <x y z>|player [d]` — move an actor to a point, or d (6) units in front of the player
- `spawn <fourcc> [n] [d <dist>] [ancs <id>|u<id> [char [anim]]] [fsm <id>] [model <id>|u<id>] [inactive] [prop <pid> <v>]` — create script objects via their loader [rules <id>] knockback RULE (patterned default 393ca543; ffffffff = none) [conn <state> <msg> <id>] add a script connection (names as obj prints them) [wpsc <id>] projectile WPSC (CHOG's two; else prop ef485db9; CHOG default 681f3907)
- `touch <mover id> <other id>` — call the mover's CollidedWith with the other actor's material list, the way the collision code does (no real collision needed)
- `pirateattach <pirate> <id>|-` — AttachActorToPirate(id) (- : DetachActorFromPirate) through the module's table, as a Metroid latching on does (coverage)
- `material <id> [+|-]<m> ...` — show or add/remove an actor's materials (name w/o kMT_, or bit; -solid) with x y z the yaw (and pitch, roll) in degrees may follow
- `shoot <id> [charged|missile] [d] [dir x y z]` — fire the beam in hand (or a real player missile) at an actor from d (3) units; travelling along dir (default from the player): a real projectile
- `struggle <0..1>|off` — hold the player's attached-actor struggle (1 wakes a grabber's dizzy gore; the game only grows it from held turn/jump buttons)
- `visor [name|0-3]` — show or switch the visor (combat, echo, scan, dark)
- `beam [name|0-3]` — show or switch the beam (power, dark, light, annihilator)
- `tp <x> <y> <z>` — teleport the player, keeping its facing
- `ray <x> <y> <z> [len] [dx dy dz]` — static and render surfaces (beam stops/passes) and the world hit along the ray (default: down)
- `face <yaw deg>` — turn the player where it stands (0 = +y, 90 = +x)
- `aim <yaw> <pitch>` — point the view outright, in degrees; negative pitch looks down
- `items` — power-up amounts and capacities
- `world <world> [area]` — load another world (the MP1 console's `warp <world>`)
- `worlds` — the memory card's world ids, areas and name tables
- `strg <id|name> [index]` — a string table as the game loads it
- `give <item> [n] | take` — add an item (capacity and amount), or remove capacity (also `take`)
- `press <a+b+...> [updates]` — hold pad buttons (0 updates: until the next press/stick)
- `grapple <id> [d]` — wake a grapple point, stand d (8) units from it facing it, hold L
- `scan <id> [d] [updates]` — scan an actor: d (40) units away, orbit locked, L held (alone: progress)
- `stick <x> <y> [updates]` — hold the main stick, -127..127
- `cstick <x> <y> [updates]` — hold the C-stick (Spring Ball in Morph Ball), -127..127
- `screenshot` — the F12 capture; the log names the file it wrote
- `at <update> <command>` — run one command on that game update (`frame`), not on arrival, so runs at different speeds act on the same tick
- `gallery [mask]` — the slideshow gallery unlock mask (0..63): print, or set via PercentScans/mode flags
- `help` — this list
- `freecam [0|1]` — free camera (photo mode); no argument toggles it
- `photo [1|2|4]` — save a PNG from the free camera, supersampled by the factor
- `livesplit [0|1|addr <host:port>|send <line>]` — the LiveSplit link (the F1 setting); no argument: status
- `touchact <actor id> <other id>` — run an actor's touch handler for the other actor (coverage)
- `trigcall <id> <trigger name> [arg]` — run one named trigger function of an actor once
- `uevent <id> <type> [dt]` — call an actor's user-event handler with an event type (coverage)
- `title` — quit to the title: the game restarts as after the pause menu's Quit
- `window [<width> <height>]` — the window size in pixels; with two numbers, leave fullscreen and resize
- `save export [<path>]` — write the card's save file out (default: save-backups/)
- `save import <path|latest>` — put a .gci back on the card; the file it replaces is kept
- `save backup | backups` — copy the card now, or list the copies in save-backups
- `a; b; c` — run several commands in the same game tick (one reply each)

## Not ported from the MP1 console

`fx` (particle generator spawning and the `fx list|tree|stats|addons` queries) has no MP2 backend:
the port has no `PortFx` effect registry or room-effect layer here. MP1 commands built on other
MP1-only systems are likewise absent; see the cfg1 report for the full list.
