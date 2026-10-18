#pragma once

// MP1's presentation rules (platform/include/port_presentation_rules.h there), the part the camera
// blend uses: free of game types (docs/FRAME_INTERPOLATION.md in the Prime 1 port).

// A tick snapshot is only valid in the tick that wrote it. A tick that skips the writer (the
// camera manager's Update in a pause) must not replay the previous pair every frame.
inline bool PortSnapshotFresh(unsigned snapshotGeneration, unsigned tickGeneration) {
  return tickGeneration != 0 && snapshotGeneration == tickGeneration;
}

// A teleport is a step over 4 units that is also over three times the step of the tick before
// (`lastStep2`, squared; 0 when unknown), so a fast chase camera is not cut every other tick.
inline bool PortIsTeleport(float dist2, float lastStep2) {
  return dist2 > 16.f && dist2 > 9.f * lastStep2;
}
