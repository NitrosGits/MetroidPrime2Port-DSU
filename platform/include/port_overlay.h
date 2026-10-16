#pragma once

#include <cstdint>

// The F1 debug overlay: a small ImGui window with the port's live settings and the warp
// buttons, so the knobs do not need environment variables and the game can be driven around
// without walking. Aurora creates the ImGui context and feeds it the SDL events; the overlay
// only has to build its windows between `aurora_begin_frame` and `aurora_end_frame`.

namespace PortOverlay {

// Published by the game every update, so the window can say where the player is. The ids are
// the game's own (a world asset id and an area index); resolving them to a name would mean
// loading the world's string table, which is not worth it for a debug label.
void SetGameState(uint32_t worldId, int areaId);
// Toggles the window; safe to call from the event loop (F1).
void Toggle();
// Debug setting (`MP2_SKIP_CINEMATICS=1`): cinematics end as soon as they start, including ones
// the player has never seen, which the game otherwise refuses to skip. Testing only - it skips
// story beats along with the intro.
bool SkipCinematics();
void SetSkipCinematics(bool skip);
bool Visible();
// Builds the window. Call once per presented frame, inside the open Aurora frame.
void Draw();

} // namespace PortOverlay
