// Port-only: model draw diagnostics, so that a broken model names itself instead of being guessed
// from a screenshot. Every model - a CMDL (CModel) and every world-geometry model of an area
// (CCubeRenderer::AddStaticGeometry) - is a CCubeModel, and every surface of one is drawn through
// CCubeModel::DrawSurface / DrawSurfaceFlat / DrawFlat, which is where `[draw]` hooks in.
//
//   MP2_DEBUG_DRAW=1      `[draw] <label> xf=(t) scale=(s) flags=0x.. n=<surface draws> v=<verdict>`
//                         once per model per MP2_DEBUG_DRAW_EVERY seconds (default 1).
//   MP2_DEBUG_DRAW=bad    the same, only for draws whose verdict is not `ok`.
//   MP2_DEBUG_MODEL_BOUNDS=1|bad
//                         `[model] <label> header=<aabb> computed=<aabb> ...` once per model build:
//                         the display lists are decoded with each surface's own vertex descriptor,
//                         the indexed positions are unioned, and indices past the end of their
//                         array, non-finite positions, bad matrix indices and display lists that
//                         do not decode are flagged. Also `[anim]` for animations of zero length.
//
// Verdict words: MTX (model matrix non-finite), VIEW (view matrix non-finite), SCALE (a model
// matrix axis outside 0.001..1000), FAR (translation beyond 1e5), POSE (a bone transform
// non-finite or out of the scale band), DL (the load-time scan of this model flagged something),
// UV(xN) (N surface draws whose TEX0 indices only fit the packed lightmap UVs ran with packed
// lightmaps off, i.e. read past the float UV array).

#include "Kyoto/Graphics/PortDrawDebug.hpp"

#include "Kyoto/Basics/CBasics.hpp"
#include "Kyoto/CPakFile.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/Graphics/CCubeMaterial.hpp"
#include "Kyoto/Graphics/CCubeModel.hpp"
#include "Kyoto/Graphics/CCubeSurface.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/Graphics/CModelFlags.hpp"
#include "Kyoto/Math/CTransform4f.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

namespace PortDrawDebug {
signed char sDrawState = -1;

namespace {
bool sInit = false;
bool sDrawBadOnly = false;
int sBoundsState = 0; // 0 off, 1 all, 2 bad only
double sEvery = 1.0;
std::mutex sMutex;

enum { kNumAttrs = 11 }; // POS, NRM, CLR0, CLR1, TEX0..TEX7
const char* const kAttrNames[kNumAttrs] = {"pos", "nrm", "c0", "c1", "t0", "t1",
                                           "t2",  "t3",  "t4", "t5", "t6"};

struct SInfo {
  std::string mLabel;
  uint mAssetId = 0;
  bool mHasAsset = false;
  bool mNameLooked = false;
  bool mBoundsPrinted = false;
  bool mScanned = false;
  bool mScanBad = false;
  bool mT0NeedsPacked = false; // TEX0 indices only fit the packed (lightmap) UV array
  // Surfaces whose TEX0 indices only fit the packed array; a draw of one with packed lightmaps
  // off reads past the float UV array (counted in mUvBad until the next print).
  std::unordered_map< const void*, uint > mPackedOnlySurfaces;
  int mUvBad = 0;
  std::string mScanLine; // the `[model]` body, without the label
  double mLastPrint = -1.0e9;
  int mDraws = 0;
  int mPoseBones = -1;
  int mPoseBadBone = -1;
};

std::unordered_map< const CCubeModel*, SInfo >& Infos() {
  static std::unordered_map< const CCubeModel*, SInfo > sInfos;
  return sInfos;
}

double Now() {
  using namespace std::chrono;
  return duration< double >(steady_clock::now().time_since_epoch()).count();
}

void EnsureInit() {
  if (sInit) {
    return;
  }
  sInit = true;
  const char* draw = std::getenv("MP2_DEBUG_DRAW");
  sDrawState = (draw != nullptr && draw[0] != '\0' && std::strcmp(draw, "0") != 0) ? 1 : 0;
  sDrawBadOnly = draw != nullptr && std::strcmp(draw, "bad") == 0;
  if (const char* every = std::getenv("MP2_DEBUG_DRAW_EVERY")) {
    const double value = std::atof(every);
    if (value > 0.0) {
      sEvery = value;
    }
  }
  const char* bounds = std::getenv("MP2_DEBUG_MODEL_BOUNDS");
  if (bounds != nullptr && bounds[0] != '\0' && std::strcmp(bounds, "0") != 0) {
    sBoundsState = std::strcmp(bounds, "bad") == 0 ? 2 : 1;
  }
  if (sDrawState != 0 || sBoundsState != 0) {
    std::fprintf(stderr, "[draw] diagnostics on: draw=%s every=%.2fs bounds=%s\n",
                 sDrawState == 0 ? "off" : (sDrawBadOnly ? "bad" : "all"), sEvery,
                 sBoundsState == 0 ? "off" : (sBoundsState == 2 ? "bad" : "all"));
  }
}

bool Finite(float v) { return std::isfinite(v); }

float ReadBEFloat(const uchar* p) {
  uint bits;
  std::memcpy(&bits, p, sizeof(bits));
  bits = CBasics::SwapBytes(bits);
  float value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

// 0 = sane; otherwise a bit set: 1 non-finite, 2 axis length outside 0.001..1000, 4 far.
int CheckTransform(const CTransform4f& xf, float* scaleOut) {
  const float* m = reinterpret_cast< const float* >(&xf);
  int result = 0;
  for (int i = 0; i < 12; ++i) {
    if (!Finite(m[i])) {
      result |= 1;
    }
  }
  for (int c = 0; c < 3; ++c) {
    const float len = std::sqrt(m[c] * m[c] + m[4 + c] * m[4 + c] + m[8 + c] * m[8 + c]);
    if (scaleOut != nullptr) {
      scaleOut[c] = len;
    }
    if (!(len >= 0.001f && len <= 1000.f)) {
      result |= 2;
    }
  }
  if (std::fabs(m[3]) > 1.0e5f || std::fabs(m[7]) > 1.0e5f || std::fabs(m[11]) > 1.0e5f) {
    result |= 4;
  }
  return result;
}

std::string LookupName(uint id) {
  if (gpResourceFactory == nullptr) {
    return std::string();
  }
  CResLoader& loader = gpResourceFactory->GetResLoader();
  const int count = loader.GetPakCount();
  for (int i = 0; i < count; ++i) {
    const CPakFile* pak = loader.GetPakFile(i);
    if (pak == nullptr || pak->IsStashedInARAM()) {
      continue;
    }
    const rstl::vector< rstl::pair< rstl::string, SObjectTag > >& names =
        pak->GetStringToObjectList();
    for (int j = 0; j < names.size(); ++j) {
      if (names[j].second.id == id) {
        return std::string(names[j].first.data());
      }
    }
  }
  return std::string();
}

void ResolveName(SInfo& info) {
  if (!info.mHasAsset || info.mNameLooked) {
    return;
  }
  info.mNameLooked = true;
  const std::string name = LookupName(info.mAssetId);
  if (!name.empty()) {
    info.mLabel += " " + name.substr(0, 48);
  }
}

// Decodes every display list of `model` with its surface's vertex descriptor.
void Scan(const CCubeModel* model, SInfo& info) {
  info.mScanned = true;
  const CCubeModel::ModelInstance& inst = model->GetModelInstance();
  const uchar* positions = static_cast< const uchar* >(inst.GetVertexPointer());
  const bool shortNormals = (model->GetModelFlags() & 1) != 0;
  uint counts[kNumAttrs];
  counts[0] = inst.GetVertexSize() / 12;
  counts[1] = inst.GetNormalSize() / (shortNormals ? 6 : 12);
  counts[2] = inst.GetColorSize() / 4;
  counts[3] = 0; // CCubeModel never sets a CLR1 array
  const uint floatUvs = inst.GetTCSize() / 8;
  const uint packedUvs = inst.GetPackedTCPointer() != nullptr ? inst.GetPackedTCSize() / 4 : 0;
  counts[4] = floatUvs > packedUvs ? floatUvs : packedUvs;
  for (int i = 5; i < kNumAttrs; ++i) {
    counts[i] = floatUvs;
  }

  uint maxIdx[kNumAttrs] = {};
  bool used[kNumAttrs] = {};
  uint oob[kNumAttrs] = {};
  uint verts = 0;
  uint prims = 0;
  uint nonFinite = 0;
  uint pnMin = 255;
  uint pnMax = 0;
  uint pnBad = 0;
  uint fmts = 0;
  char err[96] = "";
  float lo[3] = {INFINITY, INFINITY, INFINITY};
  float hi[3] = {-INFINITY, -INFINITY, -INFINITY};

  const rstl::vector< void* >& surfaces = inst.Surfaces();
  for (int s = 0; s < surfaces.size(); ++s) {
    const CCubeSurface surface(surfaces[s]);
    const uint desc = model->GetMaterial(surface).GetVertexDesc();
    uint sizes[kNumAttrs];
    uint vsize = 0;
    uint mtxBytes = 0;
    for (int b = 0; b < 8; ++b) {
      mtxBytes += (desc >> (24 + b)) & 1;
    }
    vsize = mtxBytes;
    bool direct = false;
    for (int a = 0; a < kNumAttrs; ++a) {
      const uint type = (desc >> (a * 2)) & 3;
      sizes[a] = type == 2 ? 1 : (type == 3 ? 2 : 0);
      direct |= type == 1;
      vsize += sizes[a];
      used[a] |= sizes[a] != 0;
    }
    if (direct || vsize == 0) {
      if (err[0] == '\0') {
        std::snprintf(err, sizeof(err), "s%d desc=0x%08x %s", s, desc,
                      direct ? "has a direct attr" : "is empty");
      }
      continue;
    }

    const uchar* dl = static_cast< const uchar* >(surface.GetDisplayList());
    const uint size = surface.GetDisplayListSize();
    const uint t0Before = maxIdx[4];
    maxIdx[4] = 0;
    uint p = 0;
    while (p < size) {
      const uchar cmd = dl[p];
      if (cmd == 0) {
        ++p;
        continue;
      }
      const uchar prim = cmd & 0xf8;
      if (prim < 0x80 || prim > 0xb8 || prim == 0x88) {
        if (err[0] == '\0') {
          std::snprintf(err, sizeof(err), "s%d badcmd 0x%02x at %u/%u desc=0x%08x", s, cmd, p,
                        size, desc);
        }
        break;
      }
      if (p + 3 > size) {
        if (err[0] == '\0') {
          std::snprintf(err, sizeof(err), "s%d truncated at %u/%u", s, p, size);
        }
        break;
      }
      const uint count = (static_cast< uint >(dl[p + 1]) << 8) | dl[p + 2];
      p += 3;
      if (static_cast< unsigned long long >(count) * vsize > size - p) {
        if (err[0] == '\0') {
          std::snprintf(err, sizeof(err), "s%d overrun %u*%u at %u/%u desc=0x%08x", s, count,
                        vsize, p, size, desc);
        }
        break;
      }
      fmts |= 1u << (cmd & 7);
      ++prims;
      for (uint v = 0; v < count; ++v, p += vsize) {
        const uchar* vp = dl + p;
        if (desc & (1u << 24)) {
          const uint pn = vp[0];
          pnMin = pn < pnMin ? pn : pnMin;
          pnMax = pn > pnMax ? pn : pnMax;
          if (pn % 3 != 0 || pn >= 30) {
            ++pnBad;
          }
        }
        uint off = mtxBytes;
        for (int a = 0; a < kNumAttrs; ++a) {
          if (sizes[a] == 0) {
            continue;
          }
          const uint idx = sizes[a] == 1 ? vp[off] : (static_cast< uint >(vp[off]) << 8) | vp[off + 1];
          off += sizes[a];
          maxIdx[a] = idx > maxIdx[a] ? idx : maxIdx[a];
          if (idx >= counts[a]) {
            ++oob[a];
            continue;
          }
          if (a == 0 && positions != nullptr) {
            const uchar* pos = positions + idx * 12;
            const float xyz[3] = {ReadBEFloat(pos), ReadBEFloat(pos + 4), ReadBEFloat(pos + 8)};
            if (!Finite(xyz[0]) || !Finite(xyz[1]) || !Finite(xyz[2])) {
              ++nonFinite;
              continue;
            }
            for (int k = 0; k < 3; ++k) {
              lo[k] = xyz[k] < lo[k] ? xyz[k] : lo[k];
              hi[k] = xyz[k] > hi[k] ? xyz[k] : hi[k];
            }
          }
        }
        ++verts;
      }
    }
    if (packedUvs != 0 && sizes[4] != 0 && maxIdx[4] >= floatUvs) {
      info.mPackedOnlySurfaces[surfaces[s]] = maxIdx[4];
    }
    maxIdx[4] = maxIdx[4] > t0Before ? maxIdx[4] : t0Before;
  }

  const CAABox& box = model->GetBoundingBox();
  const float hlo[3] = {box.GetMinPoint().GetX(), box.GetMinPoint().GetY(), box.GetMinPoint().GetZ()};
  const float hhi[3] = {box.GetMaxPoint().GetX(), box.GetMaxPoint().GetY(), box.GetMaxPoint().GetZ()};
  bool headerBad = false;
  bool headerNone = true;
  float diag2 = 0.f;
  for (int k = 0; k < 3; ++k) {
    headerNone &= hlo[k] == 0.f && hhi[k] == 0.f;
    headerBad |= !Finite(hlo[k]) || !Finite(hhi[k]) || hlo[k] > hhi[k];
    diag2 += (hhi[k] - hlo[k]) * (hhi[k] - hlo[k]);
  }
  const float diag = std::sqrt(diag2) > 1.0e-3f ? std::sqrt(diag2) : 1.0e-3f;
  float outside = 0.f;
  const bool haveComputed = lo[0] <= hi[0];
  headerBad &= !headerNone;
  if (haveComputed && !headerBad && !headerNone) {
    for (int k = 0; k < 3; ++k) {
      const float below = hlo[k] - lo[k];
      const float above = hi[k] - hhi[k];
      outside = below > outside ? below : outside;
      outside = above > outside ? above : outside;
    }
  }
  const float outsideRatio = outside / diag;
  const bool mismatch = outsideRatio > 0.05f;

  char oobText[160] = "";
  uint oobTotal = 0;
  for (int a = 0; a < kNumAttrs; ++a) {
    oobTotal += oob[a];
    if (oob[a] != 0) {
      const size_t len = std::strlen(oobText);
      std::snprintf(oobText + len, sizeof(oobText) - len, "%s%s:%u>=%u(x%u)", len ? "," : "",
                    kAttrNames[a], maxIdx[a], counts[a], oob[a]);
    }
  }
  info.mT0NeedsPacked = packedUvs != 0 && used[4] && maxIdx[4] >= floatUvs;
  if (info.mT0NeedsPacked) {
    const size_t len = std::strlen(oobText);
    std::snprintf(oobText + len, sizeof(oobText) - len, "%st0 packed-only:%u>=%u", len ? "," : "",
                  maxIdx[4], floatUvs);
  }

  info.mScanBad = mismatch || headerBad || nonFinite != 0 || oobTotal != 0 || pnBad != 0 ||
                  err[0] != '\0';
  char pnText[48];
  if (pnMin > pnMax) {
    std::snprintf(pnText, sizeof(pnText), "none");
  } else {
    std::snprintf(pnText, sizeof(pnText), "%u..%u%s", pnMin, pnMax, pnBad ? "(BAD)" : "");
  }
  char line[640];
  if (haveComputed) {
    std::snprintf(line, sizeof(line),
                  "header=(%.4g,%.4g,%.4g)-(%.4g,%.4g,%.4g) computed=(%.4g,%.4g,%.4g)-(%.4g,%.4g,%.4g)"
                  " out=%.1f%% surf=%d prims=%u verts=%u arrays=%u/%u/%u/%u pnmtx=%s fmt=0x%02x"
                  " nonfinite=%u oob=%s dl=%s => %s",
                  hlo[0], hlo[1], hlo[2], hhi[0], hhi[1], hhi[2], lo[0], lo[1], lo[2], hi[0], hi[1],
                  hi[2], outsideRatio * 100.f, surfaces.size(), prims, verts, counts[0], counts[1],
                  counts[2], floatUvs, pnText, fmts, nonFinite, oobText[0] ? oobText : "none",
                  err[0] ? err : "ok", info.mScanBad ? (mismatch ? "MISMATCH" : "BAD") : "OK");
  } else {
    std::snprintf(line, sizeof(line),
                  "header=(%.4g,%.4g,%.4g)-(%.4g,%.4g,%.4g) computed=none surf=%d prims=%u"
                  " verts=%u arrays=%u/%u/%u/%u nonfinite=%u oob=%s dl=%s => %s",
                  hlo[0], hlo[1], hlo[2], hhi[0], hhi[1], hhi[2], surfaces.size(), prims, verts,
                  counts[0], counts[1], counts[2], floatUvs, nonFinite, oobText[0] ? oobText : "none",
                  err[0] ? err : "ok", info.mScanBad ? "BAD" : "OK");
  }
  info.mScanLine = line;
}

void PrintBounds(SInfo& info) {
  if (sBoundsState == 0 || info.mBoundsPrinted || !info.mScanned) {
    return;
  }
  info.mBoundsPrinted = true;
  if (sBoundsState == 2 && !info.mScanBad) {
    return;
  }
  ResolveName(info);
  std::fprintf(stderr, "[model] %s %s\n", info.mLabel.c_str(), info.mScanLine.c_str());
}
} // namespace

void OnModelCreated(const CCubeModel* model) {
  EnsureInit();
  if (sDrawState == 0 && sBoundsState == 0) {
    return;
  }
  std::lock_guard< std::mutex > lock(sMutex);
  SInfo& info = Infos()[model];
  info = SInfo();
  char label[48];
  std::snprintf(label, sizeof(label), "cube@%p#%d", static_cast< const void* >(model),
                model->GetModelIndex());
  info.mLabel = label;
  Scan(model, info);
}

void OnModelDestroyed(const CCubeModel* model) {
  if (!sInit || (sDrawState == 0 && sBoundsState == 0)) {
    return;
  }
  std::lock_guard< std::mutex > lock(sMutex);
  Infos().erase(model);
}

void LabelAsset(const CCubeModel* model, uint assetId) {
  if (!sInit || (sDrawState == 0 && sBoundsState == 0) || model == nullptr) {
    return;
  }
  std::lock_guard< std::mutex > lock(sMutex);
  SInfo& info = Infos()[model];
  char label[32];
  std::snprintf(label, sizeof(label), "CMDL 0x%08X", assetId);
  info.mLabel = label;
  info.mAssetId = assetId;
  info.mHasAsset = true;
  PrintBounds(info);
}

void LabelWorld(const CCubeModel* model, int areaId, int index) {
  if (!sInit || (sDrawState == 0 && sBoundsState == 0) || model == nullptr) {
    return;
  }
  std::lock_guard< std::mutex > lock(sMutex);
  SInfo& info = Infos()[model];
  char label[32];
  std::snprintf(label, sizeof(label), "area%d/world%d", areaId, index);
  info.mLabel = label;
  PrintBounds(info);
}

void LabelGuiFrame(const CCubeModel* model, int index) {
  if (!sInit || (sDrawState == 0 && sBoundsState == 0) || model == nullptr) {
    return;
  }
  std::lock_guard< std::mutex > lock(sMutex);
  SInfo& info = Infos()[model];
  char label[48];
  std::snprintf(label, sizeof(label), "guiframe%d@%p", index, static_cast< const void* >(model));
  info.mLabel = label;
  PrintBounds(info);
}

void CopyLabel(const CCubeModel* from, const CCubeModel* to) {
  if (!sInit || (sDrawState == 0 && sBoundsState == 0) || from == nullptr || to == nullptr) {
    return;
  }
  std::lock_guard< std::mutex > lock(sMutex);
  std::unordered_map< const CCubeModel*, SInfo >::iterator it = Infos().find(from);
  if (it == Infos().end()) {
    return;
  }
  SInfo& dst = Infos()[to];
  dst.mLabel = it->second.mLabel;
  dst.mAssetId = it->second.mAssetId;
  dst.mHasAsset = it->second.mHasAsset;
  dst.mNameLooked = it->second.mNameLooked;
  dst.mBoundsPrinted = true; // the original build already reported it
}

void NotePose(const CCubeModel* model, const CTransform4f* bones, int count) {
  EnsureInit();
  if (sDrawState == 0 || model == nullptr) {
    return;
  }
  int bad = -1;
  if (bones != nullptr) {
    for (int i = 0; i < count; ++i) {
      if (CheckTransform(bones[i], nullptr) & 3) {
        bad = i;
        break;
      }
    }
  }
  std::lock_guard< std::mutex > lock(sMutex);
  SInfo& info = Infos()[model];
  info.mPoseBones = count;
  if (bad >= 0 || info.mPoseBadBone < 0) {
    info.mPoseBadBone = bad; // a bad bone sticks until the next print
  }
}

void OnDraw(const CCubeModel* model, const CModelFlags* flags, const void* surface) {
  EnsureInit();
  if (sDrawState == 0 || model == nullptr) {
    return;
  }
  std::lock_guard< std::mutex > lock(sMutex);
  std::unordered_map< const CCubeModel*, SInfo >::iterator it = Infos().find(model);
  if (it == Infos().end()) {
    char label[48];
    std::snprintf(label, sizeof(label), "cube@%p#%d", static_cast< const void* >(model),
                  model->GetModelIndex());
    it = Infos().insert(std::make_pair(model, SInfo())).first;
    it->second.mLabel = label;
  }
  SInfo& info = it->second;
  ++info.mDraws;
  if (surface != nullptr && info.mT0NeedsPacked && !CCubeModel::IsUsingPackedLightmaps() &&
      info.mPackedOnlySurfaces.find(surface) != info.mPackedOnlySurfaces.end()) {
    ++info.mUvBad;
  }
  const double now = Now();
  if (now - info.mLastPrint < sEvery) {
    return;
  }
  if (!info.mScanned) {
    Scan(model, info);
  }
  PrintBounds(info);

  const CTransform4f& xf = CGraphics::GetModelMatrix();
  float scale[3];
  const int xfCheck = CheckTransform(xf, scale);
  const int viewCheck = CheckTransform(CGraphics::GetViewMatrix(), nullptr) & 1;
  char verdict[96] = "";
  if (xfCheck & 1) std::strcat(verdict, "MTX ");
  if (viewCheck) std::strcat(verdict, "VIEW ");
  if (xfCheck & 2) std::strcat(verdict, "SCALE ");
  if (xfCheck & 4) std::strcat(verdict, "FAR ");
  char pose[40] = "";
  if (info.mPoseBones >= 0) {
    if (info.mPoseBadBone >= 0) {
      std::snprintf(pose, sizeof(pose), " pose=BAD(bone %d/%d)", info.mPoseBadBone,
                    info.mPoseBones);
      std::strcat(verdict, "POSE ");
    } else {
      std::snprintf(pose, sizeof(pose), " pose=%d", info.mPoseBones);
    }
  }
  if (info.mScanBad) std::strcat(verdict, "DL ");
  char uv[24] = "";
  if (info.mUvBad != 0) {
    std::snprintf(uv, sizeof(uv), "UV(x%d) ", info.mUvBad);
    std::strcat(verdict, uv);
  }
  const size_t vlen = std::strlen(verdict);
  if (vlen != 0) {
    verdict[vlen - 1] = '\0';
  }
  const bool ok = vlen == 0;
  const int draws = info.mDraws;
  info.mDraws = 0;
  info.mLastPrint = now;
  info.mPoseBadBone = -1;
  info.mUvBad = 0;
  if (sDrawBadOnly && ok) {
    return;
  }
  ResolveName(info);
  uint flagWord = 0;
  if (flags != nullptr) {
    flagWord = (static_cast< uint >(flags->GetTrans()) << 24) |
               (static_cast< uint >(flags->GetShaderSet() & 0xff) << 16) |
               (flags->GetOtherFlags() & 0xffff);
  }
  std::fprintf(stderr, "[draw] %s xf=(%.4g,%.4g,%.4g) scale=(%.4g,%.4g,%.4g) flags=0x%08x%s n=%d"
                       " v=%s%s\n",
               info.mLabel.c_str(), xf.Get03(), xf.Get13(), xf.Get23(), scale[0], scale[1],
               scale[2], flagWord, flags != nullptr ? "" : "(flat)", draws, ok ? "ok" : verdict,
               pose);
}

void OnAnimLoaded(uint assetId, int format, float seconds) {
  EnsureInit();
  if (sBoundsState == 0) {
    return;
  }
  if (seconds > 0.f && Finite(seconds)) {
    return;
  }
  std::lock_guard< std::mutex > lock(sMutex);
  std::string name = LookupName(assetId);
  std::fprintf(stderr, "[anim] ANIM 0x%08X%s%s format=%d duration=%g => ZERO-LENGTH\n", assetId,
               name.empty() ? "" : " ", name.substr(0, 48).c_str(), format, seconds);
}
} // namespace PortDrawDebug
