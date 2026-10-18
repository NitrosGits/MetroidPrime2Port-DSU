#pragma once

// The keyboard remap page (Prime 1's Controls > Remap, on this port's PortBinds table): every
// bindable pad input with its key, click to capture, per-row reset, "Reset all", and a marker where
// two rows share a key.
namespace PortRemapUi {

// Draws the page into the current ImGui window or child. Call it every frame while the page is
// visible; it lays out its own scrolling list, so the caller only has to give it room.
void Draw();

// True while a row is armed and waiting for a key or mouse click; the overlay should keep its own
// hotkeys (F1, Esc to close) from acting on that press.
bool Capturing();

} // namespace PortRemapUi
