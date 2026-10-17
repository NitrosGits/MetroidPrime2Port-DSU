#ifndef _PORTDRAWDEBUG
#define _PORTDRAWDEBUG

// Port-only model draw diagnostics (see src/Kyoto/Graphics/PortDrawDebug.cpp). Both switches are
// read on first use, after the platform layer has applied `env.` lines from port_paths.txt:
//   MP2_DEBUG_DRAW=1|bad          one `[draw]` line per model per MP2_DEBUG_DRAW_EVERY seconds
//                                 (default 1) with the model matrix and a sanity verdict;
//                                 `bad` prints only draws whose verdict is not ok.
//   MP2_DEBUG_MODEL_BOUNDS=1|bad  one `[model]` line per model build: the header AABB against the
//                                 AABB of the vertices its display lists index, out-of-range
//                                 indices, non-finite positions, and `[anim]` lines for animations
//                                 that parse to zero length.

#ifdef TARGET_PC
#include "types.h"

class CCubeModel;
class CModelFlags;
class CTransform4f;

namespace PortDrawDebug {
// -1 until the environment is read, then 0 (off) or 1 (on).
extern signed char sDrawState;

void OnModelCreated(const CCubeModel* model);
void OnModelDestroyed(const CCubeModel* model);
void LabelAsset(const CCubeModel* model, uint assetId);
void LabelWorld(const CCubeModel* model, int areaId, int index);
void LabelGuiFrame(const CCubeModel* model, int index);
void CopyLabel(const CCubeModel* from, const CCubeModel* to);
// `surface` is the surface's data pointer when the draw has set its material up, else null.
void OnDraw(const CCubeModel* model, const CModelFlags* flags, const void* surface = nullptr);
void NotePose(const CCubeModel* model, const CTransform4f* bones, int count);
void OnAnimLoaded(uint assetId, int format, float seconds);

inline bool DrawMaybeOn() { return sDrawState != 0; }
} // namespace PortDrawDebug
#endif

#endif // _PORTDRAWDEBUG
