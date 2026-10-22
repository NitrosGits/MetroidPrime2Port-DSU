// tools/mathdiff/harness.cpp - the port side of tools/mathdiff.py (retail vs port on random inputs).
// Built by mathdiff.py from this file plus src/Kyoto/Math/*.cpp with the game's own compile flags,
// unresolved symbols ignored (a case must not reach code outside what is linked).
//
// One Case per line: the retail address, the retail calling signature (read by mathdiff.py to set
// up the retail call) and an adapter that runs the port function on the same inputs. An address
// may appear more than once (another input range); cases are keyed by their index.
// Signature tokens, in retail register order:
//   rK:in:N    pointer to N input words          rK:io:N   N words in, read back after the call
//   rK:out:N   pointer to N output words (zeroed) rK:i      int argument
//   fK         float argument (fK:d: a double, not rounded to single)
//   ret:f / ret:i / ret:b   f1 / r3 (signed) / r3 & 0xFF
//   buffer words are floats; a 4th field picks another kind: :i int32, :d double (8 bytes),
//   :u floats normalized as one vector (unit-vector arguments), :rot a random rotation matrix
//   (9 = CMatrix3f, 12 = CTransform4f with a random translation)
//   any token may end in @lo,hi[,step] to set the random range (default -10,10; step quantizes,
//   so @0,1,1 gives 0 or 1 and equality tests see both arms)
//   a token `nospecial` turns off the NaN/inf/zero injection for that case
// Adapter: x = the inputs in token order (in/io buffers' words, then each scalar where it appears);
// y = the outputs in token order (io and out buffers), then the return value.
// Objects are built from and read back to their raw words (F/I/D below), so no member names are
// needed: a class's host layout must equal retail's (CHECK_SIZEOF holds that for these classes).
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/Math/CCylinder.hpp"
#include "Kyoto/Math/CLine.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CMatrix3f.hpp"
#include "Kyoto/Math/CMatrix4f.hpp"
#include "Kyoto/Math/CNUQuaternion.hpp"
#include "Kyoto/Math/CPlane.hpp"
#include "Kyoto/Math/CQuad.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CSphere.hpp"
#include "Kyoto/Math/CTransform4f.hpp"
#include "Kyoto/Math/CTri.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "Kyoto/Math/CVector2f.hpp"
#include "Kyoto/Math/CVector2i.hpp"
#include "Kyoto/Math/CVector3d.hpp"
#include "Kyoto/Math/CVector3f.hpp"
#include "Kyoto/Math/CVector3i.hpp"
#include "Kyoto/Math/CloseEnough.hpp"

namespace {

// A T from its words: floats (F), int32 (I) or doubles (D); P* write them back.
template < class T, class W >
T Make(const double* x) {
  W w[sizeof(T) / sizeof(W)];
  for (unsigned i = 0; i < sizeof(T) / sizeof(W); ++i) w[i] = W(x[i]);
  alignas(T) unsigned char b[sizeof(T)];
  std::memcpy(b, w, sizeof(T));
  return *reinterpret_cast< T* >(b);
}
template < class W, class T >
void Put(double* y, const T& t) {
  W w[sizeof(T) / sizeof(W)];
  std::memcpy(w, &t, sizeof(T));
  for (unsigned i = 0; i < sizeof(T) / sizeof(W); ++i) y[i] = double(w[i]);
}
template < class T > T F(const double* x) { return Make< T, float >(x); }
template < class T > T I(const double* x) { return Make< T, int >(x); }
template < class T > T D(const double* x) { return Make< T, double >(x); }
template < class T > void PF(double* y, const T& t) { Put< float >(y, t); }
template < class T > void PI(double* y, const T& t) { Put< int >(y, t); }
template < class T > void PD(double* y, const T& t) { Put< double >(y, t); }

typedef CVector3f V3;
typedef CVector2f V2;
typedef CQuaternion Q;
typedef CNUQuaternion NUQ;
typedef CMatrix3f M3;
typedef CMatrix4f M4;
typedef CTransform4f T4;
typedef CAABox Box;
typedef CRelAngle Ang;
typedef CUnitVector3f U3;

static_assert(sizeof(M3) == 0x24 && sizeof(T4) == 0x30 && sizeof(M4) == 0x40 && sizeof(Q) == 0x10 &&
              sizeof(NUQ) == 0x10 && sizeof(Box) == 0x18 && sizeof(CPlane) == 0x10 &&
              sizeof(CTri) == 0x34 && sizeof(CQuad) == 0x40 && sizeof(CLine) == 0x18 &&
              sizeof(CCylinder) == 0x1C && sizeof(CSphere) == 0x10 && sizeof(Ang) == 4 &&
              sizeof(CVector3d) == 0x18 && sizeof(CVector2i) == 8 && sizeof(CVector3i) == 0xC,
              "host layout differs from retail");

typedef void (*Fn)(const double* x, double* y);
struct Case {
  std::uint32_t addr;
  const char* sig;
  Fn fn;
};

#define A [](const double* x, double* y)
// Signatures reused below
#define S_V3_V3 "r3:out:3 r4:in:3"
#define S_SPLINE "r3:out:3 r4:in:3 r5:in:3 r6:in:3 r7:in:3 f1@-0.5,1.5"
#define SPL(fn) A { PF(y, CMath::fn(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6), F< V3 >(x + 9), float(x[12]))); }

const Case kCases[] = {
    // --- CloseEnough ----------------------------------------------------------------------
    {0x802C60EC, "r3:in:2@-1e-4,1e-4 r4:in:2@-1e-4,1e-4 f1@0,2e-4 ret:b",
     A { y[0] = close_enough(F< V2 >(x), F< V2 >(x + 2), float(x[4])); }},
    {0x802C6134, "r3:in:3@-1e-4,1e-4 r4:in:3@-1e-4,1e-4 f1@0,2e-4 ret:b",
     A { y[0] = close_enough(F< V3 >(x), F< V3 >(x + 3), float(x[6])); }},
    {0x802C61A8, "r3:in:4@0,1,1 r4:in:4@0,1,1 ret:b", A { y[0] = (F< Q >(x) == F< Q >(x + 4)); }},
    {0x802C61F8, "r3:in:4@-1e-4,1e-4 r4:in:4@-1e-4,1e-4 f1@0,2e-4 ret:b",
     A { y[0] = close_enough(F< Q >(x), F< Q >(x + 4), float(x[8])); }},

    // --- CMatrix3f ------------------------------------------------------------------------
    {0x802C62C4, "r3:out:9 r4:in:9", A { M3 m = F< M3 >(x); M3 r = F< M3 >(x); r = m; PF(y, r); }},
    {0x802C631C, "r3:io:9 r4:in:9 f1", A { M3 m = F< M3 >(x); m.AddScaledMatrix(F< M3 >(x + 9), float(x[18])); PF(y, m); }},
    {0x802C63B0, "r3:in:9 ret:f", A { y[0] = F< M3 >(x).Determinant(); }},
    {0x802C63FC, "r3:out:9 r4:in:9 r5:in:9", A { PF(y, F< M3 >(x) * F< M3 >(x + 9)); }},
    {0x802C6540, "r3:out:3 r4:in:9 r5:in:3", A { PF(y, F< M3 >(x) * F< V3 >(x + 9)); }},
    {0x802C65A4, "r3:out:9 r4:in:9", A { PF(y, F< M3 >(x).Orthonormalized()); }},
    {0x802C6690, "r3:out:9 r4:in:1@-7,7", A { PF(y, M3::RotateZ(F< Ang >(x))); }},
    {0x802C671C, "r3:out:9 r4:in:1@-7,7", A { PF(y, M3::RotateY(F< Ang >(x))); }},
    {0x802C67A8, "r3:out:9 r4:in:1@-7,7", A { PF(y, M3::RotateX(F< Ang >(x))); }},
    {0x802C6834, "r3:out:9 r4:in:3 r5:in:3 r6:in:3",
     A { PF(y, M3::FromColumns(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6))); }},
    {0x802C6924, "r3:out:9 r4:in:9 f1 r5:in:9 f2",
     A { PF(y, M3(F< M3 >(x), float(x[9]), F< M3 >(x + 10), float(x[19]))); }},
    {0x802C69DC, "r3:out:9 r4:in:9 f1", A { PF(y, M3(F< M3 >(x), float(x[9]))); }},
    {0x802C6A4C, "r3:out:9 r4:in:3 r5:in:3 r6:in:3",
     A { PF(y, M3(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6))); }},

    // --- CMatrix4f ------------------------------------------------------------------------
    {0x802C6AF0, "r3:in:16 r4:in:3 ret:f", A { y[0] = F< M4 >(x).MultiplyGetW(F< V3 >(x + 16)); }},
    {0x802C6B20, "r3:out:3 r4:in:16 r5:in:3", A { PF(y, F< M4 >(x).MultiplyOneOverW(F< V3 >(x + 16))); }},
    {0x802C6BD0, "r3:out:3 r4:in:16 r5:in:3", A { PF(y, F< M4 >(x) * F< V3 >(x + 16)); }},

    // --- CNUQuaternion --------------------------------------------------------------------
    {0x802C6D24, "r3:out:4 r4:in:4", A { PF(y, Q::FromNUQuaternion(F< NUQ >(x))); }},
    {0x802C6D74, "r3:out:4 f1 r4:in:4", A { PF(y, float(x[0]) * F< NUQ >(x + 1)); }},
    {0x802C6DD0, "r3:out:4 r4:in:4 f1", A { PF(y, F< NUQ >(x) * float(x[4])); }},
    {0x802C6E2C, "r3:out:4 r4:in:9", A { PF(y, NUQ::BuildFromMatrix3f(F< M3 >(x))); }},
    // a rotation matrix too, so every trace arm sees the inputs it is written for
    {0x802C6E2C, "r3:out:4 r4:in:9:rot", A { PF(y, NUQ::BuildFromMatrix3f(F< M3 >(x))); }},
    {0x802C7050, "r3:out:4 r4:in:4", A { PF(y, NUQ::BuildFromQuaternion(F< Q >(x))); }},
    {0x802C7078, "r3:out:4 r4:in:3 f1", A { PF(y, NUQ::BuildFromAxisAngle(F< V3 >(x), float(x[3]))); }},
    {0x802C7128, "r3:out:4 r4:in:4", A { PF(y, F< NUQ >(x).AsNormalized()); }},
    {0x802C71D0, "r3:io:4 r4:in:4", A { NUQ q = F< NUQ >(x); q += F< NUQ >(x + 4); PF(y, q); }},
    {0x802C7214, "r3:out:4 r4:in:4 r5:in:4", A { PF(y, F< NUQ >(x) * F< NUQ >(x + 4)); }},
    {0x802C734C, "r3:out:4 f1 f2 f3 f4", A { PF(y, NUQ(float(x[0]), float(x[1]), float(x[2]), float(x[3]))); }},
    {0x802C7360, "r3:out:4 f1 r4:in:3", A { PF(y, NUQ(float(x[0]), F< V3 >(x + 1))); }},

    // --- CQuaternion ----------------------------------------------------------------------
    {0x802C7380, "r3:out:4 r4:in:3 r5:in:3 r6:in:1@0,3.2",
     A { PF(y, Q::ShortestRotationArcClamped(F< V3 >(x), F< V3 >(x + 3), F< Ang >(x + 6))); }},
    {0x802C7458, "r3:out:4 r4:in:1@-7,7", A { PF(y, Q::ZRotation(F< Ang >(x))); }},
    {0x802C748C, "r3:out:4 r4:in:1@-7,7", A { PF(y, Q::YRotation(F< Ang >(x))); }},
    {0x802C74C0, "r3:out:4 r4:in:1@-7,7", A { PF(y, Q::XRotation(F< Ang >(x))); }},
    {0x802C74F4, "r3:out:4 r4:in:4 r5:in:4", A { PF(y, F< Q >(x) * F< Q >(x + 4)); }},
    {0x802C7608, "r3:out:3 r4:in:4:u r5:in:3", A { PF(y, F< Q >(x).Transform(F< V3 >(x + 4))); }},
    {0x802C7698, "r3:out:4 r4:in:3:u r5:in:1@-7,7", A { PF(y, Q::AxisAngle(F< U3 >(x), F< Ang >(x + 3))); }},
    // BuildInverted (src/MetroidPrime/CAnimData.cpp)
    {0x80028E08, "r3:out:4 r4:in:4", A { PF(y, F< Q >(x).BuildInverted()); }},
    {0x802C773C, "r3:out:4 r4:in:4", A { PF(y, F< Q >(x).BuildNormalized()); }},
    {0x802C77D0, "r3:out:1 r4:in:4:u r5:in:4:u", A { PF(y, F< Q >(x).AngleFrom(F< Q >(x + 4))); }},
    {0x802C785C, "r3:out:4 r4:in:4:u r5:in:4:u f1@-0.2,1.2",
     A { PF(y, Q::SlerpLocal(F< Q >(x), F< Q >(x + 4), float(x[8]))); }},
    {0x802C7924, "r3:out:4 r4:in:3:u r5:in:3:u r6:in:1@0,3.2",
     A { PF(y, Q::LookAt(F< U3 >(x), F< U3 >(x + 3), F< Ang >(x + 6))); }},
    {0x802C7D24, "r3:out:4 r4:in:3 r5:in:3", A { PF(y, Q::ShortestRotationArc(F< V3 >(x), F< V3 >(x + 3))); }},
    // near-(anti)parallel pairs reach the degenerate arms
    {0x802C7D24, "r3:out:4 r4:in:3@-1,1,1 r5:in:3@-1,1,1",
     A { PF(y, Q::ShortestRotationArc(F< V3 >(x), F< V3 >(x + 3))); }},
    {0x802C7F8C, "r3:out:4 r4:in:4:u r5:in:4:u f1@-0.2,1.2",
     A { PF(y, Q::Slerp(F< Q >(x), F< Q >(x + 4), float(x[8]))); }},
    {0x802C7F8C, "r3:out:4 r4:in:4@-1,1,1 r5:in:4@-1,1,1 f1@0,1",
     A { PF(y, Q::Slerp(F< Q >(x), F< Q >(x + 4), float(x[8]))); }},
    {0x802C8154, "r3:out:12 r4:in:4 r5:in:3", A { PF(y, F< Q >(x).BuildTransform4f(F< V3 >(x + 4))); }},
    {0x802C8250, "r3:out:12 r4:in:4", A { PF(y, F< Q >(x).BuildTransform4f()); }},
    {0x802C8324, "r3:out:9 r4:in:4", A { PF(y, F< Q >(x).BuildTransform()); }},
    {0x802C83C0, "r3:out:4 r4:in:12", A { PF(y, Q::FromMatrix(F< T4 >(x))); }},
    {0x802C8418, "r3:out:4 r4:in:9", A { PF(y, Q::FromMatrix(F< M3 >(x))); }},
    {0x802C8418, "r3:out:4 r4:in:9:rot", A { PF(y, Q::FromMatrix(F< M3 >(x))); }},
    {0x802C83C0, "r3:out:4 r4:in:12:rot", A { PF(y, Q::FromMatrix(F< T4 >(x))); }},
    {0x802C8470, "r3:out:4 r4:in:3 r5:in:3 r6:in:3",
     A { PF(y, Q::FromMatrixColumns(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6))); }},
    {0x802C86C8, "r3:out:4 r4:in:3 r5:in:3 r6:in:3",
     A { PF(y, Q::FromMatrixRows(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6))); }},

    // --- CTransform4f ---------------------------------------------------------------------
    {0x802C8BB8, "r3:out:12 r4:in:12:rot", A { PF(y, F< T4 >(x).GetInverse()); }},
    {0x802C8BB8, "r3:out:12 r4:in:12", A { PF(y, F< T4 >(x).GetInverse()); }},
    {0x802C8E30, "r3:out:12 r4:in:12 r5:in:12", A { PF(y, F< T4 >(x) * F< T4 >(x + 12)); }},
    {0x802C8F04, "r3:out:3 r4:in:12 r5:in:3", A { PF(y, F< T4 >(x).TransposeRotate(F< V3 >(x + 12))); }},
    {0x802C8F60, "r3:out:3 r4:in:12 r5:in:3", A { PF(y, F< T4 >(x).Rotate(F< V3 >(x + 12))); }},
    {0x802C8FC8, "r3:out:3 r4:in:12 r5:in:3", A { PF(y, F< T4 >(x) * F< V3 >(x + 12)); }},
    {0x802C9020, "r3:out:12 r4:in:12", A { T4 r = F< T4 >(x); r = F< T4 >(x); PF(y, r); }},
    {0x802C9088, "r3:io:12 r4:in:12", A { T4 t = F< T4 >(x); t.SetRotation(F< T4 >(x + 12)); PF(y, t); }},
    {0x802C90D4, "r3:out:12 r4:in:12", A { PF(y, F< T4 >(x).GetRotation()); }},
    {0x802C9130, "r3:out:12 r4:in:12", A { PF(y, F< T4 >(x).GetQuickInverse()); }},
    {0x802C91BC, "r3:out:12 r4:in:3 r5:in:3 r6:in:3 r7:in:3",
     A { PF(y, T4::FromRows(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6), F< V3 >(x + 9))); }},
    {0x802C9220, "r3:out:12 r4:in:3 r5:in:3 r6:in:3 r7:in:3",
     A { PF(y, T4::FromColumns(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6), F< V3 >(x + 9))); }},
    {0x802C9284, "r3:io:12 f1", A { T4 t = F< T4 >(x); t.ScaleBy(float(x[12])); PF(y, t); }},
    {0x802C92F4, "r3:out:12 r4:in:12 r5:in:12", A { PF(y, F< T4 >(x).MultiplyIgnoreTranslation(F< T4 >(x + 12))); }},
    {0x802C9458, "r3:out:9 r4:in:12", A { PF(y, F< T4 >(x).BuildMatrix3f()); }},
    {0x802C94A4, "r3:out:12 r4:in:3", A { PF(y, T4::Translate(F< V3 >(x))); }},
    {0x802C94EC, "r3:out:12 f1 f2 f3", A { PF(y, T4::Translate(float(x[0]), float(x[1]), float(x[2]))); }},
    {0x802C9528, "r3:out:12 r4:in:3", A { PF(y, T4::Scale(F< V3 >(x))); }},
    {0x802C956C, "r3:out:12 f1 f2 f3", A { PF(y, T4::Scale(float(x[0]), float(x[1]), float(x[2]))); }},
    {0x802C95A4, "r3:out:12 f1", A { PF(y, T4::Scale(float(x[0]))); }},
    {0x802C95DC, "r3:out:12 r4:in:9 r5:in:3", A { PF(y, T4(F< M3 >(x), F< V3 >(x + 9))); }},
    {0x802C9708, "r3:io:12:rot", A { T4 t = F< T4 >(x); t.Orthonormalize(); PF(y, t); }},
    {0x802C9708, "r3:io:12", A { T4 t = F< T4 >(x); t.Orthonormalize(); PF(y, t); }},
    {0x802C9828, "r3:io:12 r4:in:1@-7,7", A { T4 t = F< T4 >(x); t.RotateLocalZ(F< Ang >(x + 12)); PF(y, t); }},
    {0x802C9944, "r3:io:12 r4:in:1@-7,7", A { T4 t = F< T4 >(x); t.RotateLocalY(F< Ang >(x + 12)); PF(y, t); }},
    {0x802C9A60, "r3:io:12 r4:in:1@-7,7", A { T4 t = F< T4 >(x); t.RotateLocalX(F< Ang >(x + 12)); PF(y, t); }},
    {0x802C9B7C, "r3:out:12 r4:in:1@-7,7", A { PF(y, T4::RotateZ(F< Ang >(x))); }},
    {0x802C9C14, "r3:out:12 r4:in:1@-7,7", A { PF(y, T4::RotateY(F< Ang >(x))); }},
    {0x802C9CAC, "r3:out:12 r4:in:1@-7,7", A { PF(y, T4::RotateX(F< Ang >(x))); }},
    {0x802C9D44, "r3:out:12 f1 f2 f3 f4 f5 f6", A {
       PF(y, T4::Shear(float(x[0]), float(x[1]), float(x[2]), float(x[3]), float(x[4]), float(x[5])));
     }},
    {0x802C9D80, "r3:out:12 r4:in:3:u", A { PF(y, T4::MakeRotationsBasedOnY(F< U3 >(x))); }},
    // the Z-up arm: a nearly vertical Y
    {0x802C9D80, "r3:out:12 r4:in:3:u@-0.01,0.01,0.01", A { PF(y, T4::MakeRotationsBasedOnY(F< U3 >(x))); }},
    {0x802C9E5C, "r3:out:12 r4:in:3 r5:in:3 r6:in:3:u",
     A { PF(y, T4::LookAt(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6))); }},
    {0x802C9E5C, "r3:out:12 r4:in:3@-1,1,1 r5:in:3@-1,1,1 r6:in:3@-1,1,1",
     A { PF(y, T4::LookAt(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6))); }},

    // --- CUnitVector3f --------------------------------------------------------------------
    {0x802CA12C, S_V3_V3, A { PF(y, U3(F< V3 >(x))); }},

    // --- CVector2f ------------------------------------------------------------------------
    {0x802CA1C0, "r3:out:2 r4:in:2 r5:in:1", A { PF(y, F< V2 >(x) / float(x[2])); }},
    {0x802CA1FC, "r3:out:2 r4:in:1 r5:in:2", A { PF(y, float(x[0]) * F< V2 >(x + 1)); }},
    {0x802CA230, "r3:out:2 r4:in:2 r5:in:1", A { PF(y, F< V2 >(x) * float(x[2])); }},
    {0x802CA264, "r3:in:2@0,1,1 r4:in:2@0,1,1 ret:b", A { y[0] = (F< V2 >(x) == F< V2 >(x + 2)); }},
    {0x802CA294, "r3:out:2 r4:in:2 r5:in:2", A { PF(y, F< V2 >(x) - F< V2 >(x + 2)); }},
    {0x802CA2CC, "r3:out:2 r4:in:2 r5:in:2", A { PF(y, F< V2 >(x) + F< V2 >(x + 2)); }},
    {0x802CA304, "r3:in:2 r4:in:2 ret:f", A { y[0] = V2::Dot(F< V2 >(x), F< V2 >(x + 2)); }},
    {0x802CA320, "r3:in:2 r4:in:2 ret:f", A { y[0] = V2::GetAngleDiff(F< V2 >(x), F< V2 >(x + 2)); }},
    {0x802CA3E4, "r3:out:2 r4:in:2", A { PF(y, F< V2 >(x).AsNormalized()); }},
    {0x802CA440, "r3:in:2 ret:f", A { y[0] = F< V2 >(x).MagSquared(); }},
    {0x802CA458, "r3:in:2 ret:f", A { y[0] = F< V2 >(x).Magnitude(); }},
    {0x802CA48C, "r3:io:2 f1", A { V2 v = F< V2 >(x); v /= float(x[2]); PF(y, v); }},
    {0x802CA4B0, "r3:io:2 f1", A { V2 v = F< V2 >(x); v *= float(x[2]); PF(y, v); }},
    {0x802CA4CC, "r3:io:2 r4:in:2", A { V2 v = F< V2 >(x); v -= F< V2 >(x + 2); PF(y, v); }},
    {0x802CA4F0, "r3:io:2 r4:in:2", A { V2 v = F< V2 >(x); v += F< V2 >(x + 2); PF(y, v); }},
    {0x802CA514, "r3:io:2", A { V2 v = F< V2 >(x); v.Normalize(); PF(y, v); }},
    {0x802CA514, "r3:io:2@-1e-20,1e-20", A { V2 v = F< V2 >(x); v.Normalize(); PF(y, v); }},
    {0x802CA564, "r3:out:2 f1 f2", A { PF(y, V2(float(x[0]), float(x[1]))); }},

    // --- CVector2i (divisor kept off 0: divw by 0 is undefined on both sides) --------------
    {0x802CA5B0, "r3:out:2:i r4:in:2:i@-1000,1000 r5:i@1,20", A { PI(y, I< CVector2i >(x) / int(x[2])); }},
    {0x802CA5E0, "r3:out:2:i r4:in:2:i@-1000,1000 r5:i", A { PI(y, I< CVector2i >(x) * int(x[2])); }},
    {0x802CA610, "r3:in:2:i@0,1 r4:in:2:i@0,1 ret:b", A { y[0] = (I< CVector2i >(x) != I< CVector2i >(x + 2)); }},
    {0x802CA640, "r3:in:2:i@0,1 r4:in:2:i@0,1 ret:b", A { y[0] = (I< CVector2i >(x) == I< CVector2i >(x + 2)); }},
    {0x802CA670, "r3:out:2:i r4:in:2:i r5:in:2:i", A { PI(y, I< CVector2i >(x) - I< CVector2i >(x + 2)); }},
    {0x802CA6A8, "r3:out:2:i r4:in:2:i r5:in:2:i", A { PI(y, I< CVector2i >(x) + I< CVector2i >(x + 2)); }},
    {0x802CA6E0, "r3:out:2:i r4:i r5:i", A { PI(y, CVector2i(int(x[0]), int(x[1]))); }},

    // --- CVector3d ------------------------------------------------------------------------
    {0x802CA6EC, "r3:out:3:d f1:d r4:in:3:d", A { PD(y, x[0] * D< CVector3d >(x + 1)); }},
    {0x802CA728, "r3:out:3:d r4:in:3:d", A { PD(y, -D< CVector3d >(x)); }},
    {0x802CA760, "r3:out:3:d r4:in:3:d r5:in:3:d", A { PD(y, D< CVector3d >(x) - D< CVector3d >(x + 3)); }},
    {0x802CA7A4, "r3:out:3:d r4:in:3:d r5:in:3:d", A { PD(y, D< CVector3d >(x) + D< CVector3d >(x + 3)); }},
    {0x802CA7E8, "r3:out:3:d r4:in:3:d r5:in:3:d", A { PD(y, CVector3d::Cross(D< CVector3d >(x), D< CVector3d >(x + 3))); }},
    {0x802CA838, "r3:in:3:d r4:in:3:d ret:f", A { y[0] = CVector3d::Dot(D< CVector3d >(x), D< CVector3d >(x + 3)); }},
    {0x802CA860, "r3:out:3 r4:in:3:d", A { PF(y, D< CVector3d >(x).AsCVector3f()); }},
    {0x802CA888, "r3:out:3:d r4:in:3:d", A { PD(y, D< CVector3d >(x).AsNormalized()); }},
    {0x802CA8EC, "r3:in:3:d ret:f", A { y[0] = D< CVector3d >(x).MagSquared(); }},
    {0x802CA910, "r3:in:3:d ret:f", A { y[0] = D< CVector3d >(x).Magnitude(); }},
    {0x802CA950, "r3:out:3:d r4:in:3", A { PD(y, CVector3d(F< V3 >(x))); }},
    {0x802CA96C, "r3:out:3:d f1:d f2:d f3:d", A { PD(y, CVector3d(x[0], x[1], x[2])); }},

    // --- CVector3f ------------------------------------------------------------------------
    {0x802CAB00, "r3:in:3@-1e-3,1e-3 r4:in:3@-1e-3,1e-3 f1@0,2e-3 ret:b",
     A { y[0] = F< V3 >(x).IsEqu(F< V3 >(x + 3), float(x[6])); }},
    {0x802CAB80, "r3:in:3 r4:in:3 ret:f", A { y[0] = V3::GetAngleDiff(F< V3 >(x), F< V3 >(x + 3)); }},
    {0x802CAC4C, "r3:in:3 ret:b", A { y[0] = F< V3 >(x).CanBeNormalized(); }},
    {0x802CAC4C, "r3:in:3@-1e-14,1e-14 ret:b", A { y[0] = F< V3 >(x).CanBeNormalized(); }},
    {0x802CACE0, "r3:in:3 ret:b", A { y[0] = F< V3 >(x).IsMagnitudeSafe(); }},
    {0x802CACE0, "r3:in:3@-1e-14,1e-14 ret:b", A { y[0] = F< V3 >(x).IsMagnitudeSafe(); }},
    {0x802CAD58, "r3:in:3 ret:b", A { y[0] = F< V3 >(x).IsNotInf(); }},
    {0x802CADA4, S_V3_V3, A { PF(y, F< V3 >(x).AsNormalized()); }},
    {0x802CAE0C, "r3:in:3 ret:f", A { y[0] = F< V3 >(x).Magnitude(); }},
    {0x802CAE4C, "r3:io:3", A { V3 v = F< V3 >(x); v.Normalize(); PF(y, v); }},
    {0x802CAEA8, "r3:out:3 r4:in:3 r5:in:3 r6:in:1@-3.2,3.2",
     A { PF(y, V3::Slerp(F< V3 >(x), F< V3 >(x + 3), F< Ang >(x + 6))); }},

    // --- CVector3i ------------------------------------------------------------------------
    {0x802CB124, "r3:in:3:i@0,1 r4:in:3:i@0,1 ret:b", A { y[0] = (I< CVector3i >(x) == I< CVector3i >(x + 3)); }},
    {0x802CB164, "r3:out:3:i r4:i r5:i r6:i", A { PI(y, CVector3i(int(x[0]), int(x[1]), int(x[2]))); }},

    // --- CMath (RMathUtils) ---------------------------------------------------------------
    {0x802CB330, "f1@-0.2,1.2 r3:i@0,1 f2@0,1 f3@0,1 f4 f5 f6@0,3 ret:f", A {
       y[0] = CMath::EaseInOut(float(x[0]), CMath::EEaseTypes(int(x[1])), float(x[2]), float(x[3]),
                               float(x[4]), float(x[5]), float(x[6]));
     }},
    {0x802CB588, "f1@-1.2,1.2 f2@0,8 ret:f", A { y[0] = CMath::PhongBlob(float(x[0]), float(x[1])); }},
    {0x802CB608, "f1:d f2:d f3:d f4:d f5:d@0,1e-6 r3:out:3:d ret:i", A {
       double r[3] = {};
       const int n = CMath::SolveCubicDouble(x[0], x[1], x[2], x[3], x[4], r);
       for (int i = 0; i < 3; ++i) y[i] = r[i];
       y[3] = n;
     }},
    {0x802CB608, "f1:d@-2,2,1 f2:d@-2,2,1 f3:d@-2,2,1 f4:d@-1,1,1 f5:d@0,1e-6 r3:out:3:d ret:i", A {
       double r[3] = {};
       const int n = CMath::SolveCubicDouble(x[0], x[1], x[2], x[3], x[4], r);
       for (int i = 0; i < 3; ++i) y[i] = r[i];
       y[3] = n;
     }},
    {0x802CB918, "r3:in:5 r4:out:4 ret:i", A {
       float c[5], r[4] = {};
       for (int i = 0; i < 5; ++i) c[i] = float(x[i]);
       const int n = CMath::SolveQuartic(c, r);
       for (int i = 0; i < 4; ++i) y[i] = r[i];
       y[4] = n;
     }},
    {0x802CB918, "r3:in:5@-2,2,1 r4:out:4 ret:i", A {
       float c[5], r[4] = {};
       for (int i = 0; i < 5; ++i) c[i] = float(x[i]);
       const int n = CMath::SolveQuartic(c, r);
       for (int i = 0; i < 4; ++i) y[i] = r[i];
       y[4] = n;
     }},
    {0x802CBCA0, "r3:in:4 r4:out:3 ret:i", A {
       float c[4], r[3] = {};
       for (int i = 0; i < 4; ++i) c[i] = float(x[i]);
       const int n = CMath::SolveCubic(c, r);
       for (int i = 0; i < 3; ++i) y[i] = r[i];
       y[3] = n;
     }},
    {0x802CBCA0, "r3:in:4@-2,2,1 r4:out:3 ret:i", A {
       float c[4], r[3] = {};
       for (int i = 0; i < 4; ++i) c[i] = float(x[i]);
       const int n = CMath::SolveCubic(c, r);
       for (int i = 0; i < 3; ++i) y[i] = r[i];
       y[3] = n;
     }},
    {0x802CC064, "f1 f2 f3 r3:out:1 r4:out:1 ret:b", A {
       float p = 0.f, m = 0.f;
       y[2] = CMath::SolveQuadratic(float(x[0]), float(x[1]), float(x[2]), p, m);
       y[0] = p;
       y[1] = m;
     }},
    {0x802CC120, "r3:i@1,2000000000 ret:i", A { y[0] = CMath::FloorLog2(uint(x[0])); }},
    {0x802CC120, "r3:i@1,70 ret:i", A { y[0] = CMath::FloorLog2(uint(x[0])); }},
    {0x802CC194, "r3:i@0,2000000000 ret:i", A { y[0] = CMath::FloorPowerOfTwo(int(x[0])); }},
    {0x802CC194, "r3:i@-5,70 ret:i", A { y[0] = CMath::FloorPowerOfTwo(int(x[0])); }},
    {0x802CC210, "f1@-1.1,1.1 ret:f", A { y[0] = CMath::FastArcCosR(float(x[0])); }},
    {0x802CC284, "f1@-20,20 ret:f", A { y[0] = CMath::FastCosR(float(x[0])); }},
    {0x802CC330, "f1@-20,20 ret:f", A { y[0] = CMath::FastSinR(float(x[0])); }},
    {0x802CC3DC, "r3:out:3 r4:in:3 r5:in:3 r6:in:3 r7:in:3",
     A { PF(y, CMath::BaryToWorld(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6), F< V3 >(x + 9))); }},
    {0x802CC458, "f1@-50,50 f2@-50,50 f3@-50,50 f4@-50,50 ret:f",
     A { y[0] = CMath::Noise4d(float(x[0]), float(x[1]), float(x[2]), float(x[3])); }},
    {0x802CC478, "f1@-50,50 f2@-50,50 f3@-50,50 ret:f", A { y[0] = CMath::Noise3d(float(x[0]), float(x[1]), float(x[2])); }},
    {0x802CC498, "f1@-50,50 f2@-50,50 ret:f", A { y[0] = CMath::Noise2d(float(x[0]), float(x[1])); }},
    {0x802CC4BC, "f1@-50,50 ret:f", A { y[0] = CMath::Noise1d(float(x[0])); }},
    {0x802CCE4C, S_SPLINE, SPL(GetBSplineTangent)},
    {0x802CCF88, S_SPLINE, SPL(GetBSplinePoint)},
    {0x802CD0E0, S_SPLINE, SPL(GetBezierTangent)},
    {0x802CD1E4, S_SPLINE, SPL(GetBezierPoint)},
    {0x802CD2E4, S_SPLINE, SPL(GetRoundedCatmullRomSplineTangent)},
    {0x802CD5B4, S_SPLINE, SPL(GetRoundedCatmullRomSplinePoint)},
    {0x802CD8DC, "f1 f2 f3 f4 f5@-0.5,1.5 ret:f", A {
       y[0] = CMath::GetCatmullRomSplinePoint(float(x[0]), float(x[1]), float(x[2]), float(x[3]), float(x[4]));
     }},
    {0x802CD988, S_SPLINE, SPL(GetCatmullRomSplineTangent)},
    {0x802CDAC4, S_SPLINE, SPL(GetCatmullRomSplinePoint)},
    {0x802CDC50, S_SPLINE, SPL(GetHermiteSplineTangent)},
    {0x802CDD54, S_SPLINE, SPL(GetHermiteSplinePoint)},
    {0x802CDE98, "f1@-100,100 ret:f", A { y[0] = CMath::CeilingF(float(x[0])); }},
    {0x802CDE98, "f1@-3,3,0.5 ret:f", A { y[0] = CMath::CeilingF(float(x[0])); }},
    {0x802CDEE8, "f1@0,100 ret:f", A { y[0] = CMath::InvSqrtF(float(x[0])); }},
    {0x802CDF10, "f1:d@0,100 ret:f", A { y[0] = CMath::SqrtD(x[0]); }},
    {0x802CDF30, "f1@0,100 ret:f", A { y[0] = CMath::SqrtF(float(x[0])); }},

    // --- CCylinder / CLine / CPlane / CTri / CQuad / CSphere ------------------------------
    {0x802F74C4, "r3:in:7 r4:in:3 ret:b", A { y[0] = F< CCylinder >(x).PointInside(F< V3 >(x + 7)); }},
    {0x802F7578, "r3:out:3 r4:in:7 r5:in:3", A { PF(y, F< CCylinder >(x).GetAxisPoint(F< V3 >(x + 7))); }},
    {0x802F75A4, "r3:out:3 r4:in:7 r5:in:3", A { PF(y, F< CCylinder >(x).GetSurfacePoint(F< V3 >(x + 7))); }},
    {0x802F76C4, "r3:out:3 r4:in:6 r5:in:3", A { PF(y, F< CLine >(x).GetClosestPoint(F< V3 >(x + 6))); }},
    {0x802F7728, "r3:out:3 r4:in:4 r5:in:3", A { PF(y, F< CPlane >(x).GetClosestPoint(F< V3 >(x + 4))); }},
    {0x802F777C, "r3:in:4 r4:in:3 r5:in:3 ret:f",
     A { y[0] = F< CPlane >(x).ClipLineSegment(F< V3 >(x + 4), F< V3 >(x + 7)); }},
    {0x802F7868, "r3:out:4 r4:in:3 r5:in:3 r6:in:3", A { PF(y, CPlane(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6))); }},
    {0x802F7938, "r3:out:13 r4:in:16 r5:i@-1,6", A { PF(y, F< CQuad >(x).GetTri(int(x[16]))); }},
    {0x802F79EC, "r3:out:13 r4:in:3 r5:in:3 r6:in:3", A { PF(y, CTri(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6))); }},
    {0x802F7A88, "r3:out:16 r4:in:3 r5:in:3 r6:in:3 r7:in:3",
     A { PF(y, CQuad(F< V3 >(x), F< V3 >(x + 3), F< V3 >(x + 6), F< V3 >(x + 9))); }},
    {0x802F7B28, "r3:out:3 r4:in:4 r5:in:3", A { PF(y, F< CSphere >(x).GetSurfacePoint(F< V3 >(x + 4))); }},
    {0x802F7C14, "r3:out:3 r4:in:4 r5:in:3", A { PF(y, F< CSphere >(x).GetSurfaceNormal(F< V3 >(x + 4))); }},

    // --- CAABox ---------------------------------------------------------------------------
    {0x802F7C68, "r3:out:3 r4:in:6 r5:in:3", A { PF(y, F< Box >(x).ClosestPointAlongVector(F< V3 >(x + 6))); }},
    {0x802F7CA8, "r3:out:3 r4:in:6 r5:in:3", A { PF(y, F< Box >(x).FurthestPointAlongVector(F< V3 >(x + 6))); }},
    {0x802F7CE8, "r3:in:6 r4:in:6 ret:f", A { y[0] = Box::DistanceBetween(F< Box >(x), F< Box >(x + 6)); }},
    {0x802F7FC0, "r3:in:6 r4:in:3 ret:b", A { y[0] = F< Box >(x).PointInside(F< V3 >(x + 6)); }},
    {0x802F803C, "r3:out:3 r4:in:6 r5:i@0,7", A { PF(y, F< Box >(x).GetPoint(int(x[6]))); }},
    {0x802F8098, "r3:out:3 r4:in:6", A { PF(y, F< Box >(x).GetCenterPoint()); }},
    {0x802F80DC, "r3:in:6 ret:f", A { y[0] = F< Box >(x).GetVolume(); }},
    {0x802F810C, "r3:in:6 ret:b", A { y[0] = F< Box >(x).Invalid(); }},
    {0x802F81B0, "r3:out:6 r4:in:6 r5:in:6", A { PF(y, F< Box >(x).GetBooleanIntersection(F< Box >(x + 6))); }},
    {0x802F8310, "r3:out:3 r4:in:6 r5:in:3", A { PF(y, F< Box >(x).ClampToBox(F< V3 >(x + 6))); }},
    {0x802F8398, "r3:out:6 r4:in:6 r5:in:12", A { PF(y, F< Box >(x).GetTransformedAABox(F< T4 >(x + 6))); }},
    {0x802F84C0, "r3:in:6 r4:in:4 ret:b", A { y[0] = F< Box >(x).InsidePlane(F< CPlane >(x + 6)); }},
    {0x802F851C, "r3:in:6 r4:in:6 ret:b", A { y[0] = F< Box >(x).Inside(F< Box >(x + 6)); }},
    {0x802F85A4, "r3:io:6 r4:in:3", A { Box b = F< Box >(x); b.AccumulateBounds(F< V3 >(x + 6)); PF(y, b); }},
    {0x802F8620, "r3:in:6 r4:in:6 ret:b", A { y[0] = F< Box >(x).DoBoundsOverlap(F< Box >(x + 6)); }},
    {0x802F8690, "r3:out:16 r4:in:6 r5:i@0,5", A { PF(y, F< Box >(x).GetQuad(Box::EBoxFaceId(int(x[6])))); }},
    {0x802F8960, "r3:out:13 r4:in:6 r5:i@0,5 r6:i@0,3",
     A { PF(y, F< Box >(x).GetTri(Box::EBoxFaceId(int(x[6])), int(x[7]))); }},
    {0x802F8BA0, "r3:out:2:i r4:i@0,11", A { PI(y, Box::GetEdge(Box::EBoxEdgeId(int(x[0])))); }},
    {0x802F8CC4, "r3:out:6 r4:in:3 r5:in:3", A { PF(y, Box(F< V3 >(x), F< V3 >(x + 3))); }},
};

} // namespace

int main(int argc, char** argv) {
  const int ncases = int(sizeof kCases / sizeof kCases[0]);
  if (argc > 1 && std::strcmp(argv[1], "--list") == 0) {
    for (int i = 0; i < ncases; ++i) std::printf("%d 0x%08" PRIX32 " %s\n", i, kCases[i].addr, kCases[i].sig);
    return 0;
  }
  // stdin: `CASE NIN NOUT x0 .. x(NIN-1)` per line; stdout: NOUT outputs as %a, one line per input line
  static char line[1 << 16];
  while (std::fgets(line, sizeof line, stdin)) {
    char* p = line;
    const int idx = int(std::strtol(p, &p, 0));
    const int n = int(std::strtol(p, &p, 0));
    const int nout = int(std::strtol(p, &p, 0));
    double x[256] = {}, y[256];
    for (int i = 0; i < n && i < 256; ++i) x[i] = std::strtod(p, &p);
    for (double& v : y) v = __builtin_nan("");
    if (idx < 0 || idx >= ncases) {
      std::printf("nocase\n");
      continue;
    }
    kCases[idx].fn(x, y);
    for (int i = 0; i < nout && i < 256; ++i) std::printf("%s%a", i ? " " : "", y[i]);
    std::printf("\n");
    std::fflush(stdout);
  }
  return 0;
}
