#include "Kyoto/Text/CRasterFont.hpp"

#include "Kyoto/CFactoryMgr.hpp"
#ifdef TARGET_PC
#include "Kyoto/Basics/CBasics.hpp"
#include "Kyoto/Math/CMath.hpp"
#endif
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/CVParamTransfer.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Graphics/CGX.hpp"
#include "Kyoto/Graphics/CGraphicsPalette.hpp"
#include "Kyoto/Graphics/CTexture.hpp"
#include "Kyoto/Streams/CInputStream.hpp"
#include "Kyoto/Text/CDrawStringOptions.hpp"
#include "Kyoto/Text/CTextRenderBuffer.hpp"
#include "Kyoto/Text/TextCommon.hpp"

#include "rstl/algorithm.hpp"
#include "rstl/string.hpp"
#include "rstl/vector.hpp"

CRasterFont::CRasterFont(CInputStream& in, IObjectStore* store)
: mInitialized(false), mMonoWidth(16), mMonoHeight(16), mMode(kFM_OneLayer), mLineMargin(0) {
  if (in.ReadInt32() == 'FONT') {
    int version = in.ReadInt32();
    if (version >= 0 && version <= 4) {
      mMonoWidth = in.ReadInt32();
      mMonoHeight = in.ReadInt32();
      if (version >= 1) {
        mBaseline = in.ReadInt32();
      } else {
        mBaseline = mMonoHeight;
      }

      if (version >= 2) {
        mLineMargin = in.ReadInt32();
      }

      bool fInfoA = in.ReadBool();
      bool fInfoB = in.ReadBool();
      int fInfoC = in.ReadInt32();
      int fontSize = in.ReadInt32();
      rstl::string fontName(in);
      CAssetId fontId = in.ReadInt32();

      if (store != nullptr) {
        mTexture = store->GetObj(SObjectTag('TXTR', fontId));
        mTexture->Lock();
      }

      mFontInfo = CFontInfo(fInfoA, fInfoB, fInfoC, fontSize, fontName.data());

      int mode = in.ReadInt32();
      switch (mode) {
      case 0:
        mMode = kFM_OneLayer;
        break;
      case 1:
        mMode = kFM_OneLayerOutline;
        break;
      case 2:
        mMode = kFM_FourLayers;
        break;
      case 3:
        mMode = kFM_TwoLayersOutline;
        break;
      case 4:
        mMode = kFM_TwoLayers;
        break;
      }

      int glyphCount = in.ReadInt32();
      mGlyphs.reserve(glyphCount);

      for (int i = 0; i < glyphCount; ++i) {
        ushort chr = in.Get< ushort >();
        float startU = in.ReadFloat();
        float startV = in.ReadFloat();
        float endU = in.ReadFloat();
        float endV = in.ReadFloat();
        int layer = version >= 3 ? in.ReadUint8() : 0;
        char a = version >= 4 ? in.ReadInt8() : char(in.ReadInt32());
        char b = version >= 4 ? in.ReadInt8() : char(in.ReadInt32());
        char c = version >= 4 ? in.ReadInt8() : char(in.ReadInt32());
        uchar cellWidth = version >= 4 ? in.ReadUint8() : uchar(in.ReadInt32());
        uchar cellHeight = version >= 4 ? in.ReadUint8() : uchar(in.ReadInt32());
        uchar baseline = version >= 4 ? in.ReadUint8() : uchar(in.ReadInt32());
        int kernStart = version >= 4 ? in.ReadUint16() : short(in.ReadInt32());
        mGlyphs.push_back_unsafe(
            rstl::pair< wchar_t, CGlyph >(chr, CGlyph(a, b, c, startU, startV, endU, endV, layer,
                                                      cellWidth, cellHeight, baseline, kernStart)));
      }

      rstl::sort_by_key(mGlyphs);
#ifdef TARGET_PC
      // Placeholder for the U+221E the shipped fonts lack; AddInfinityGlyph fills it in.
      if (InternalGetGlyph(0x221E) == nullptr) {
        mGlyphs.reserve(mGlyphs.size() + 1);
        mGlyphs.push_back_unsafe(rstl::pair< wchar_t, CGlyph >(0x221E, CGlyph(0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0, 0, 0, 0, 0)));
        rstl::sort_by_key(mGlyphs);
      }
#endif

      int kerningCount = in.ReadInt32();
      mKerning.reserve(kerningCount);

      for (int i = 0; i < kerningCount; ++i) {
        short first = in.Get< short >();
        short second = in.Get< short >();
        int howMuch = in.ReadInt32();
        mKerning.push_back_unsafe(CKernPair(first, second, howMuch));
      }

      mInitialized = true;
    }
  }
}

EFontMode CRasterFont::GetMode() const { return mMode; }

void CRasterFont::GetSize(const CDrawStringOptions& options, int& width, int& height,
                          const wchar_t* str, int length) const {
  width = 0;
  height = 0;
  int curWidth = 0;
  const CGlyph* prevGlyph = nullptr;
#ifdef TARGET_PC
  // Test the length first: callers pass unterminated one-char strings.
  for (const wchar_t* ptr = str; (length == -1 || ptr - str < length) && *ptr != 0; ++ptr) {
#else
  for (const wchar_t* ptr = str; *ptr != 0 && (length == -1 || ptr - str < length); ++ptr) {
#endif
    const CGlyph* glyph = GetGlyph(*ptr);
    if (glyph != nullptr) {
      int kerning =
          prevGlyph != nullptr ? KernLookup(mKerning, prevGlyph->GetKernStart(), *ptr) : 0;
      int newWidth = curWidth + glyph->GetA() + glyph->GetB() + glyph->GetC() + kerning +
                     options.GetCharacterExtraSpace();
      int newHeight = mMonoHeight - glyph->GetBaseline() + glyph->GetCellHeight();
      if (options.GetTextDirection() == kTD_Horizontal) {
        width = newWidth;
        curWidth = newWidth;
        if (newHeight > height) {
          height = newHeight;
        }
      }
    }
    prevGlyph = glyph;
  }
}

int CRasterFont::GetCarriageAdvance() const { return mMonoHeight + mLineMargin; }

void CRasterFont::DrawString(const CDrawStringOptions& options, int x, int y, int& xOut, int& yOut,
                             CTextRenderBuffer* buffer, const wchar_t* str, int length) const {
  if (!mInitialized) {
    return;
  }

  if (buffer != nullptr) {
    CGraphicsPalette pal(kPF_RGB5A3, 4);
    ushort* data = reinterpret_cast< ushort* >(pal.Lock());
    data[0] = CColor(0.f, 0.f, 0.f, 0.f).ToRGB5A3();
    data[1] = CColor(options.GetPaletteEntry(0)).ToRGB5A3();
    data[2] = CColor(options.GetPaletteEntry(1)).ToRGB5A3();
    data[3] = CColor(0.f, 0.f, 0.f, 0.f).ToRGB5A3();
#ifdef TARGET_PC
    // TLUT entries are read as big-endian (disc data); store these host-built ones the same way,
    // or a coloured main/outline entry decodes as a different, half-transparent colour.
    for (int i = 0; i < 4; ++i) {
      data[i] = CBasics::SwapBytes(data[i]);
    }
#endif
    pal.UnLock();
    buffer->AddPaletteChange(pal, GetMode());
  }

  SinglePassDrawString(options, x, y, xOut, yOut, buffer, str, length);
}

void CRasterFont::DrawSpace(const CDrawStringOptions& options, int x, int y, int& xOut, int& yOut,
                            int length) const {
  if (options.GetTextDirection() != kTD_Horizontal) {
    return;
  }

  xOut = x + length;
  yOut = y;
}

int CRasterFont::KernLookup(const rstl::vector< CKernPair >& kern, const int start, const int chr) {
  rstl::vector< CKernPair >::const_iterator it = kern.begin() + start;
  for (; it != kern.end() && it->GetFirst() == kern[start].GetFirst(); ++it) {
    if (it->GetSecond() == chr) {
      return it->GetHowMuch();
    }
  }

  return 0;
}

void CRasterFont::SinglePassDrawString(const CDrawStringOptions& options, const int x, const int y,
                                       int& xOut, int& yOut, CTextRenderBuffer* buffer,
                                       const wchar_t* str, const int length) const {
  if (mInitialized) {
    int curX = x;
    const CGlyph* prevGlyph = nullptr;
#ifdef TARGET_PC
    for (const wchar_t* ptr = str; (length == -1 || (ptr - str) < length) && *ptr != 0; ++ptr) {
#else
    for (const wchar_t* ptr = str; *ptr != 0 && (length == -1 || (ptr - str) < length); ++ptr) {
#endif
      const CGlyph* curGlyph = GetGlyph(*ptr);
      if (curGlyph != nullptr) {
        int xOffset = 0;
        int yOffset = 0;
        if (options.GetTextDirection() == kTD_Horizontal) {
          curX += curGlyph->GetA();
          if (prevGlyph != nullptr) {
            curX += KernLookup(mKerning, prevGlyph->GetKernStart(), *ptr);
          }
          xOffset = 0;
          yOffset = 0;
        }

        if (buffer) {
          buffer->AddCharacter(CVector2i(curX + xOffset, yOffset + (y - curGlyph->GetBaseline())),
                               *ptr, options.GetPaletteEntry(2));
        }

        if (options.GetTextDirection() == kTD_Horizontal) {
          curX += curGlyph->GetB() + curGlyph->GetC() + options.GetCharacterExtraSpace();
        }
      }
      prevGlyph = curGlyph;
    }

    xOut = curX;
    yOut = y;
  }
}

const CGlyph* CRasterFont::InternalGetGlyph(const wchar_t chr) const {
  rstl::vector< rstl::pair< wchar_t, CGlyph > >::const_iterator it =
      rstl::find_by_key(mGlyphs, chr);

  if (it == mGlyphs.end()) {
    return nullptr;
  }

#ifdef TARGET_PC
  if (chr == 0x221E && it->second.GetCellWidth() == 0) {
    const_cast< CRasterFont* >(this)->AddInfinityGlyph();
  }
#endif
  return &it->second;
}

#ifdef TARGET_PC
// Port: the shipped fonts have no U+221E (checked at runtime: the HUD digit fonts hold digits and
// Latin text only), which the infinite ammo cheat's HUD needs. The constructor registers a
// zero-size placeholder so the table never reallocates or reorders; once the font texture is
// loaded, the first lookup paints a lemniscate into a free spot of the C4 atlas and fills in the
// entry. The atlas is a stack of 1-bit planes (bit L of a texel is layer L; outline fonts keep the
// outline ring, the 8-neighbour dilation of the fill, in the next plane), so the glyph is drawn the
// same way as the digits: hard-edged, in the fill plane (and the outline plane where there is one),
// with the digit '8' as the model for stroke weight and height.
namespace {
int PortAtlasTexel(const uchar* data, int width, int x, int y) {
  const uchar b = data[((y >> 3) * (width >> 3) + (x >> 3)) * 32 + (y & 7) * 4 + ((x & 7) >> 1)];
  return (x & 1) == 0 ? b >> 4 : b & 15;
}

void PortAtlasOrTexel(uchar* data, int width, int x, int y, int bits) {
  uchar& b = data[((y >> 3) * (width >> 3) + (x >> 3)) * 32 + (y & 7) * 4 + ((x & 7) >> 1)];
  b |= uchar((x & 1) == 0 ? bits << 4 : bits);
}

// True when no texel of `mask` is set anywhere in the rectangle grown by one texel (texels outside
// the atlas count as clear).
bool PortAtlasRectFree(const uchar* data, int width, int height, int x, int y, int w, int h,
                       int mask) {
  for (int j = y; j < y + h; ++j) {
    for (int i = x; i < x + w; ++i) {
      if (i >= 0 && j >= 0 && i < width && j < height &&
          (PortAtlasTexel(data, width, i, j) & mask) != 0) {
        return false;
      }
    }
  }
  return true;
}

// Fills `fill` (cellW x cellH, row-major) with a figure-eight (Gerono lemniscate: x = cos t,
// y = sin 2t, whose two strokes cross diagonally) whose centreline is stroked `stroke` texels thick
// and centred in a w x h box offset by `margin`. A texel is set when at least `threshold` percent of
// it is covered.
void PortRasterizeLemniscate(uchar* fill, int cellW, int cellH, int margin, int w, int h,
                             float stroke, int threshold) {
  enum { kSamples = 720, kSuper = 4 };
  float px[kSamples];
  float py[kSamples];
  const float rx = (float(w) - stroke) * 0.5f;
  const float ry = (float(h) - stroke) * 0.5f;
  for (int i = 0; i < kSamples; ++i) {
    const float t = 6.2831853f * float(i) / float(kSamples);
    px[i] = float(margin) + float(w) * 0.5f + rx * CMath::SlowCosineR(t);
    py[i] = float(margin) + float(h) * 0.5f + ry * CMath::SlowSineR(2.f * t);
  }
  const float radius = stroke * 0.5f;
  for (int y = 0; y < cellH; ++y) {
    for (int x = 0; x < cellW; ++x) {
      int covered = 0;
      for (int sy = 0; sy < kSuper; ++sy) {
        for (int sx = 0; sx < kSuper; ++sx) {
          const float cx = float(x) + (float(sx) + 0.5f) / float(kSuper);
          const float cy = float(y) + (float(sy) + 0.5f) / float(kSuper);
          for (int i = 0; i < kSamples; ++i) {
            const float dx = px[i] - cx;
            const float dy = py[i] - cy;
            if (dx * dx + dy * dy <= radius * radius) {
              ++covered;
              break;
            }
          }
        }
      }
      fill[y * cellW + x] = covered * 100 >= threshold * kSuper * kSuper ? 1 : 0;
    }
  }
}

// Size of the smallest of the two largest background regions of `bits` (cellW x cellH, 1 = ink)
// that do not touch the border, i.e. the open counters of an 8 or an infinity sign; 0 when there
// are fewer than two. `bits` is clobbered.
int PortSmallestCounter(uchar* bits, int cellW, int cellH) {
  int best = 0;
  int second = 0;
  for (int y = 0; y < cellH; ++y) {
    for (int x = 0; x < cellW; ++x) {
      if (bits[y * cellW + x] != 0) {
        continue;
      }
      int stack[64 * 64];
      int top = 0;
      int area = 0;
      bool open = false;
      bits[y * cellW + x] = 2;
      stack[top++] = y * cellW + x;
      while (top > 0) {
        const int at = stack[--top];
        const int ax = at % cellW;
        const int ay = at / cellW;
        ++area;
        open = open || ax == 0 || ay == 0 || ax == cellW - 1 || ay == cellH - 1;
        const int nx[4] = {ax + 1, ax - 1, ax, ax};
        const int ny[4] = {ay, ay, ay + 1, ay - 1};
        for (int k = 0; k < 4; ++k) {
          if (nx[k] >= 0 && ny[k] >= 0 && nx[k] < cellW && ny[k] < cellH &&
              bits[ny[k] * cellW + nx[k]] == 0) {
            bits[ny[k] * cellW + nx[k]] = 2;
            stack[top++] = ny[k] * cellW + nx[k];
          }
        }
      }
      if (!open) {
        if (area > best) {
          second = best;
          best = area;
        } else if (area > second) {
          second = area;
        }
      }
    }
  }
  return second;
}
} // namespace

void CRasterFont::AddInfinityGlyph() {
  const rstl::vector< rstl::pair< wchar_t, CGlyph > >::const_iterator found =
      rstl::find_by_key(mGlyphs, wchar_t(0x221E));
  if (found == mGlyphs.end() || found->second.GetCellWidth() != 0 || !mTexture ||
      !mTexture->IsLoaded()) {
    return;
  }
  CGlyph* infinity = const_cast< CGlyph* >(&found->second);
  // One try per font: a font that has no room or model glyph keeps its placeholder.
  static const CRasterFont* sTried[16];
  static int sTriedCount = 0;
  for (int i = 0; i < sTriedCount; ++i) {
    if (sTried[i] == this) {
      return;
    }
  }
  if (sTriedCount < 16) {
    sTried[sTriedCount++] = this;
  }

  const CGlyph* eight = InternalGetGlyph(L'8');
  TLockedToken< CTexture > texture = *mTexture;
  if (eight == nullptr || texture->GetTexelFormat() != kTF_C4 || texture->GetBitsPerPixel() != 4) {
    return;
  }
  uchar* data = static_cast< uchar* >(texture->GetBitMapData(0));
  const int width = texture->GetWidth();
  const int height = texture->GetHeight();
  if (data == nullptr || (width & 7) != 0 || (height & 7) != 0) {
    return;
  }

  // Measure the '8': the ink box in its own fill plane and its stroke weight.
  const bool outlined = mMode == kFM_OneLayerOutline || mMode == kFM_TwoLayersOutline;
  const int eightLayer = eight->GetLayer();
  const int rx0 = int(eight->GetStartU() * float(width) + 0.5f);
  const int ry0 = int(eight->GetStartV() * float(height) + 0.5f);
  const int rx1 = int(eight->GetEndU() * float(width) + 0.5f);
  const int ry1 = int(eight->GetEndV() * float(height) + 0.5f);
  int ix0 = rx1, iy0 = ry1, ix1 = rx0 - 1, iy1 = ry0 - 1;
  for (int y = ry0; y < ry1 && y < height; ++y) {
    for (int x = rx0; x < rx1 && x < width; ++x) {
      if ((PortAtlasTexel(data, width, x, y) >> eightLayer & 1) != 0) {
        ix0 = x < ix0 ? x : ix0;
        iy0 = y < iy0 ? y : iy0;
        ix1 = x > ix1 ? x : ix1;
        iy1 = y > iy1 ? y : iy1;
      }
    }
  }
  const int inkW = ix1 - ix0 + 1;
  const int inkH = iy1 - iy0 + 1;
  if (inkW < 3 || inkH < 3) {
    return;
  }
  int sideRun = 0;
  for (int x = ix0; x <= ix1 && (PortAtlasTexel(data, width, x, iy0 + inkH / 4) >> eightLayer & 1);
       ++x) {
    ++sideRun;
  }
  int topRun = 0;
  for (int y = iy0; y <= iy1 && (PortAtlasTexel(data, width, ix0 + inkW / 2, y) >> eightLayer & 1);
       ++y) {
    ++topRun;
  }
  // The sign's strokes run diagonally, so the '8''s thinner run is the thickness across the stroke.
  const int thin = sideRun < topRun ? sideRun : topRun;
  const float stroke = thin < 2 ? 2.f : float(thin);

  // The '8''s counters, as the yardstick for the sign's.
  uchar eightBits[64 * 64];
  if (inkW > 64 || inkH > 64) {
    return;
  }
  for (int y = 0; y < inkH; ++y) {
    for (int x = 0; x < inkW; ++x) {
      eightBits[y * inkW + x] = uchar(PortAtlasTexel(data, width, ix0 + x, iy0 + y) >> eightLayer & 1);
    }
  }
  const int eightCounter = PortSmallestCounter(eightBits, inkW, inkH);

  // Pick the shape: 1.5-1.7 times the '8' wide and 70-80% of its height, with the stroke of the
  // '8'; the best candidate keeps two open counters as large as the '8''s with the 1-bit threshold
  // tuned for this size.
  const int margin = outlined ? 1 : 0;
  int bestW = 0, bestH = 0, bestThreshold = 50, bestScore = -1;
  for (int h = (inkH * 7 + 9) / 10; h <= (inkH * 8) / 10 + 1; ++h) {
    for (int wTenths = 15; wTenths <= 17; ++wTenths) {
      const int w = (inkW * wTenths + 5) / 10;
      if (w + 2 * margin > 64 || h + 2 * margin > 64) {
        continue;
      }
      for (int t = 0; t < 5; ++t) {
        static const int kThresholds[5] = {50, 40, 60, 30, 70};
        uchar trial[64 * 64];
        PortRasterizeLemniscate(trial, w, h, 0, w, h, stroke, kThresholds[t]);
        // A solid row all the way across would hide the crossing.
        bool bar = false;
        for (int y = 0; y < h && !bar; ++y) {
          bar = true;
          for (int x = 0; x < w; ++x) {
            bar = bar && trial[y * w + x] != 0;
          }
        }
        int counter = bar ? 0 : PortSmallestCounter(trial, w, h);
        counter = counter > eightCounter ? eightCounter : counter;
        // Counters first, then the nearest to 75% of the height, 1.6 times the width, threshold 50.
        const int hOff = (h * 40 - inkH * 30) < 0 ? inkH * 30 - h * 40 : h * 40 - inkH * 30;
        const int wOff = wTenths > 16 ? wTenths - 16 : 16 - wTenths;
        const int score = counter * 1000 - hOff * 3 - wOff * 5 - t;
        if (counter > 0 && score > bestScore) {
          bestScore = score;
          bestW = w;
          bestH = h;
          bestThreshold = kThresholds[t];
        }
      }
    }
  }
  if (bestScore < 0) {
    return;
  }
  const int w = bestW;
  const int h = bestH;
  const int cellW = w + 2 * margin;
  const int cellH = h + 2 * margin;

  // Layer (fill plane) candidates, the '8's first.
  int layerCount = 1;
  if (mMode == kFM_FourLayers) {
    layerCount = 4;
  } else if (mMode == kFM_TwoLayers) {
    layerCount = 2;
  } else if (mMode == kFM_TwoLayersOutline) {
    layerCount = 4;
  }
  const int layerStep = mMode == kFM_TwoLayersOutline ? 2 : 1;
  int px = 0, py = 0, pl = 0;
  // A crowded atlas (the outline fonts) only gets the fill bit checked on the second pass.
  for (int pass = 0; pass < 2; ++pass)
  for (int k = -1; k * layerStep < layerCount; ++k) {
    const int layer = k < 0 ? eightLayer : k * layerStep;
    if ((k >= 0 && layer == eightLayer) || layer >= layerCount || (outlined && layer + 1 > 3)) {
      continue;
    }
    const int mask = outlined && pass == 0 ? 3 << layer : 1 << layer;
    for (int y = 0; y + cellH <= height; ++y) {
      for (int x = 0; x + cellW <= width; ++x) {
        if (!PortAtlasRectFree(data, width, height, x, y, cellW, cellH, mask)) {
          continue;
        }
        px = x;
        py = y;
        pl = layer;
        goto place;
      }
    }
  }
  // The atlas is full (outline fonts pack every plane): take over the cells of glyphs the HUD never
  // draws, as long as the cell fits where they (and free space) are.
  {
    static const wchar_t kVictims[] = {L'~', L'`', L'^', L'|', L'\\', L'{', L'}', L'[', L']',
                                       L'<', L'>', L'@', L'#', L'$', L'%', L'&', L'*', L'_'};
    enum { kVictimCount = sizeof(kVictims) / sizeof(kVictims[0]) };
    CGlyph* victims[kVictimCount];
    int vx[kVictimCount], vy[kVictimCount], vw[kVictimCount], vh[kVictimCount];
    int count = 0;
    for (int v = 0; v < kVictimCount; ++v) {
      const CGlyph* g = InternalGetGlyph(kVictims[v]);
      if (g == nullptr || g == eight || g->GetCellWidth() == 0) {
        continue;
      }
      vx[count] = int(g->GetStartU() * float(width) + 0.5f);
      vy[count] = int(g->GetStartV() * float(height) + 0.5f);
      vw[count] = int(g->GetEndU() * float(width) + 0.5f) - vx[count];
      vh[count] = int(g->GetEndV() * float(height) + 0.5f) - vy[count];
      victims[count++] = const_cast< CGlyph* >(g);
    }
    const int bytes = (width >> 1) * height;
    static uchar scratch[1 << 16];
    if (bytes <= int(sizeof(scratch))) {
      for (int k = 0; k * layerStep < layerCount; ++k) {
        const int layer = k * layerStep;
        const int vmask = outlined ? 3 << layer : 1 << layer;
        for (int i = 0; i < bytes; ++i) {
          scratch[i] = data[i];
        }
        for (int v = 0; v < count; ++v) {
          if (victims[v]->GetLayer() != layer) {
            continue;
          }
          for (int j = 0; j < vh[v]; ++j) {
            for (int i = 0; i < vw[v]; ++i) {
              const int x = vx[v] + i;
              const int y = vy[v] + j;
              if (x >= 0 && y >= 0 && x < width && y < height) {
                scratch[((y >> 3) * (width >> 3) + (x >> 3)) * 32 + (y & 7) * 4 + ((x & 7) >> 1)] &=
                    uchar(~((x & 1) == 0 ? vmask << 4 : vmask));
              }
            }
          }
        }
        for (int y = 0; y + cellH <= height; ++y) {
          for (int x = 0; x + cellW <= width; ++x) {
            if (!PortAtlasRectFree(scratch, width, height, x, y, cellW, cellH, 1 << layer)) {
              continue;
            }
            for (int v = 0; v < count; ++v) {
              if (victims[v]->GetLayer() != layer || vx[v] >= x + cellW || vx[v] + vw[v] <= x ||
                  vy[v] >= y + cellH || vy[v] + vh[v] <= y) {
                continue;
              }
              for (int j = 0; j < vh[v]; ++j) {
                for (int i = 0; i < vw[v]; ++i) {
                  const int xx = vx[v] + i;
                  const int yy = vy[v] + j;
                  data[((yy >> 3) * (width >> 3) + (xx >> 3)) * 32 + (yy & 7) * 4 + ((xx & 7) >> 1)] &=
                      uchar(~((xx & 1) == 0 ? vmask << 4 : vmask));
                }
              }
              *victims[v] = CGlyph(0, 0, 0, 0.f, 0.f, 0.f, 0.f, 0, 0, 0, 0, 0);
            }
            // Whatever of the cell still holds other glyphs' ring bits is wiped too.
            px = x;
            py = y;
            pl = layer;
            goto place;
          }
        }
      }
    }
  }
  return;
place:
    uchar fill[64 * 64];
    PortRasterizeLemniscate(fill, cellW, cellH, margin, w, h, stroke, bestThreshold);
    for (int j = 0; j < cellH; ++j) {
      for (int i = 0; i < cellW; ++i) {
        bool ring = false;
        if (outlined && fill[j * cellW + i] == 0) {
          for (int dj = -1; dj <= 1 && !ring; ++dj) {
            for (int di = -1; di <= 1; ++di) {
              const int yy = j + dj;
              const int xx = i + di;
              if (yy >= 0 && xx >= 0 && yy < cellH && xx < cellW && fill[yy * cellW + xx] != 0) {
                ring = true;
                break;
              }
            }
          }
        }
        if (fill[j * cellW + i] != 0) {
          PortAtlasOrTexel(data, width, px + i, py + j, 1 << pl);
        } else if (ring) {
          PortAtlasOrTexel(data, width, px + i, py + j, 1 << (pl + 1));
        }
      }
    }
    // The '8' sits on the line at its fill bottom (offset by dy8 in outline fonts); the sign
    // is centred on the digit's height.
    const int dy8 = (iy1 + 1 - ry0) - int(eight->GetBaseline());
    int baseline = margin + h - dy8 + (inkH - h + 1) / 2;
    baseline = baseline < 0 ? 0 : baseline > 255 ? 255 : baseline;
    *infinity = CGlyph(eight->GetA(), w, eight->GetC(), float(px) / float(width),
                       float(py) / float(height), float(px + cellW) / float(width),
                       float(py + cellH) / float(height), pl, cellW, cellH, baseline,
                       eight->GetKernStart());
  texture->InitTextureObjects();
}
#endif

CFactoryFnReturn FRasterFontFactory(const SObjectTag& tag, CInputStream& in,
                                          const CVParamTransfer& xfer) {
  const rstl::rc_ptr< IVParamObj > obj = xfer.GetObj();
  CSimplePool* pool = static_cast< TObjOwnerParam< CSimplePool* >* >(obj.GetPtr())->GetData();

  return rs_new CRasterFont(in, pool);
}

void CRasterFont::SetupRenderState() {
  static const GXVtxDescList skDescList[] = {
      {GX_VA_POS, GX_DIRECT},
      {GX_VA_TEX0, GX_DIRECT},
      {GX_VA_NULL, GX_NONE},
  };

  TLockedToken< CTexture > texture = *mTexture;
  texture->Load(GX_TEXMAP0, CTexture::kCM_Clamp);
  CGX::SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_K0_A);
  CGX::SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
  CGX::SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_KONST, GX_CC_ZERO);
  CGX::SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_KONST, GX_CA_ZERO);
  CGX::SetStandardTevColorAlphaOp(GX_TEVSTAGE0);
  CGX::SetTevDirect(GX_TEVSTAGE0);
  CGX::SetVtxDescv(skDescList);
  CGX::SetNumChans(0);
  CGX::SetNumTexGens(1);
  CGX::SetNumTevStages(1);
  CGX::SetNumIndStages(0);
  CGX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
  CGX::SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
}

bool CRasterFont::IsFinishedLoading() { return mTexture && mTexture->IsLoaded(); }

void CRasterFont::SetTexture(const TToken< CTexture >& texture) {
  mTexture = texture;
  mTexture->Lock();
}
