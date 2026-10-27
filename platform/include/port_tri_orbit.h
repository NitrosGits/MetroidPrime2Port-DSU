#pragma once

// Trilogy Echoes' Targeting-zone geometry as pure functions (tests/port_tri_orbit.cpp).
// rs5mp2_p.dol: width 0x802a0dec, height 0x802a0ea8 (zone 0), centre 0x802a0f5c (docs: trilogy-scanvisor).
// Same as MP1's port_tri_orbit.h minus the lock fade, with k = 1.0f (the DOL's widescreen slot).

namespace PortTriOrbit {

// Truncated in the 640x448 basis, before the live viewport scale.
const float kExtentScale = 0.65f; // 0x805cfde8

inline int HalfWidth(float raw) { return static_cast< int >(kExtentScale * raw); }
inline int HalfHeight(float raw) { return static_cast< int >(kExtentScale * raw); }

// The pointer's zone centre on one axis: int((ndc * 0.5 + 0.5) * viewport), ndc in [-1, 1].
inline int PointerCentre(float ndc, float viewport) {
  return static_cast< int >((ndc * 0.5f + 0.5f) * viewport);
}

} // namespace PortTriOrbit
