// Port-only: the default-font half of retail `main` (src/MetroidPrime/Startup.cpp).
//
// Startup.cpp stays out of the host build because it is the console process entry (`main`,
// `CSaveRegion`, `CRelDebugSupport`); platform/main.cpp is the host one. But the font it loads
// before `InvokeCMain` is what `gpDefaultFont` points at, and `CSplashScreen::Draw` and
// `CErrorOutputWindow` dereference that with no null test. The two embedded blobs are the
// `extract:` entries of config/G2ME01/config.yml, written by the upstream build to
// build/G2ME01/include (on mp_game's include path, see CMakeLists.txt).
#include "Kyoto/Graphics/CTexture.hpp"
#include "Kyoto/Streams/CLZOInputStream.hpp"
#include "Kyoto/Streams/CMemoryInStream.hpp"
#include "Kyoto/Text/CRasterFont.hpp"
#include "MetroidPrime/CGameGlobalObjects.hpp"

#include "MetroidPrime/DefaultFontData.inc"
#include "MetroidPrime/DefaultFontTexture.inc"

const TToken< CRasterFont >* gpDefaultFont;

namespace port {

// Same body as Startup.cpp's `LoadDefaultFont`.
static CRasterFont* LoadDefaultFontObject() {
  CLZOInputStream fontStream(rs_new CMemoryInStream(sDefaultFontData, sizeof(sDefaultFontData)),
                             sizeof(sDefaultFontData), 0x1660);
  CRasterFont* font = rs_new CRasterFont(fontStream, nullptr);
  CLZOInputStream textureStream(
      rs_new CMemoryInStream(sDefaultFontTexture, sizeof(sDefaultFontTexture)),
      sizeof(sDefaultFontTexture), 0xa34);
  TToken< CTexture > texture(
      rs_new CTexture(textureStream, CTexture::kAM_Zero, CTexture::kBK_Zero));
  font->SetTexture(texture);
  return font;
}

// Retail keeps the token as a local of `main` that outlives `InvokeCMain`; the caller owns the
// returned token the same way and must free it before `CGraphicsSys` goes down. The handle is
// opaque so platform/main.cpp need not see the game's template headers.
void* LoadDefaultFont() {
  TToken< CRasterFont >* token = new TToken< CRasterFont >(LoadDefaultFontObject());
  gpDefaultFont = token;
  return token;
}

void FreeDefaultFont(void* token) {
  gpDefaultFont = nullptr;
  delete static_cast< TToken< CRasterFont >* >(token);
}

} // namespace port
