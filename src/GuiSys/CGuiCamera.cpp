#include "GuiSys/CGuiCamera.hpp"
#include "GuiSys/CGuiFrame.hpp"
#include "GuiSys/CGuiWidget.hpp"
#include "GuiSys/CGuiWidgetDrawParms.hpp"
#include "Kyoto/Alloc/CMemory.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/Math/CVector3f.hpp"

#include "Kyoto/Streams/CInputStream.hpp"
#ifdef TARGET_PC
#include <math.h>
#include "port_overlay.h"
#endif

const char* const eCamTypePerspective = "eCamTypePerspective";
const char* const eCamTypeOrthogonal = "eCamTypeOrthogonal";

CGuiWidget* CGuiCamera::Create(CGuiFrame* frame, CInputStream& in, CSimplePool* sp, uint version) {
  CGuiWidgetParms parms = ReadWidgetHeader(frame, in);
  EProjection proj = static_cast< EProjection >(in.ReadInt32());
  CGuiCamera* camera = nullptr;
  if (proj == kProjection_Perspective) {
    const float fov = in.ReadFloat();
    const float aspect = in.ReadFloat();
    const float znear = in.ReadFloat();
    const float zfar = in.ReadFloat();
    camera = rs_new CGuiCamera(parms, fov, aspect, znear, zfar);
  } else if (proj == kProjection_Orthographic) {
    const float left = in.ReadFloat();
    const float right = in.ReadFloat();
    const float top = in.ReadFloat();
    const float bottom = in.ReadFloat();
    const float znear = in.ReadFloat();
    const float zfar = in.ReadFloat();
    camera = rs_new CGuiCamera(parms, left, right, top, bottom, znear, zfar);
  }

  frame->SetFrameCamera(camera);
  camera->ParseBaseInfo(frame, in, parms, version);
  return camera;
}

CGuiCamera::CGuiCamera(const CGuiWidgetParms& parms, float fov, float aspect, float znear,
                       float zfar)
: CGuiWidget(parms) {
  mProjection = kProjection_Perspective;
  CVector3f(1.f, 0.f, 0.f).Normalize();
  mCameraParms.mPerspective.mFov = fov;
  mCameraParms.mPerspective.mAspect = aspect;
  mCameraParms.mPerspective.mNear = znear;
  mCameraParms.mPerspective.mFar = zfar;
}

CGuiCamera::CGuiCamera(const CGuiWidgetParms& parms, float left, float right, float top,
                       float bottom, float znear, float zfar)
: CGuiWidget(parms) {
  mProjection = kProjection_Orthographic;
  mCameraParms.mOrthographic.mLeft = left;
  mCameraParms.mOrthographic.mRight = right;
  mCameraParms.mOrthographic.mTop = top;
  mCameraParms.mOrthographic.mBottom = bottom;
  mCameraParms.mOrthographic.mNear = znear;
  mCameraParms.mOrthographic.mFar = zfar;
}

void CGuiCamera::Draw(const CGuiWidgetDrawParms& parms) const {
#ifdef TARGET_PC
  // Port: HUD scale (`MP2_HUD_SCALE`, F1 > HUD). Only CSamusHud::Draw sets the flag, so the pause
  // screen, the inventory, the helmet and memo screens keep the size the game authored.
  const int portHudScale = PortOverlay::HudScale();
  const bool portHudScaled =
      PortOverlay::HudScalingActive() && portHudScale != PortOverlay::kHudScaleMax;
  // Scaling the projection about its centre draws the HUD smaller around the middle of the screen
  // without moving the camera.
  const float portHudInvScale = portHudScaled ? 100.f / static_cast< float >(portHudScale) : 1.f;
  // Port: the aspect setting (PortOverlay::ApplyAspect). The frames are authored for the game's own
  // 640x448; on a wider framebuffer the projection is widened by the same factor, as the Prime 1
  // port's aspect-matched GUI cameras are, so the GUI keeps its proportions instead of stretching.
  const float portWiden = PortOverlay::AspectWiden();
#endif
  if (mProjection == kProjection_Perspective) {
#ifdef TARGET_PC
    // A wider field of view shrinks the content about the view axis, the perspective counterpart of
    // the orthographic branch below.
    float fov = mCameraParms.mPerspective.mFov;
    if (portHudScaled) {
      const float halfTan = tanf(0.5f * fov * 3.14159265f / 180.f) * portHudInvScale;
      fov = 2.f * atanf(halfTan) * 180.f / 3.14159265f;
    }
    CGraphics::SetPerspective(fov, mCameraParms.mPerspective.mAspect * portWiden,
                              mCameraParms.mPerspective.mNear, mCameraParms.mPerspective.mFar);
#else
    CGraphics::SetPerspective(mCameraParms.mPerspective.mFov, mCameraParms.mPerspective.mAspect,
                              mCameraParms.mPerspective.mNear, mCameraParms.mPerspective.mFar);
#endif
  } else {
#ifdef TARGET_PC
    if (portHudScaled || portWiden != 1.f) {
      const float left = mCameraParms.mOrthographic.mLeft;
      const float right = mCameraParms.mOrthographic.mRight;
      const float top = mCameraParms.mOrthographic.mTop;
      const float bottom = mCameraParms.mOrthographic.mBottom;
      const float midX = 0.5f * (left + right);
      const float midY = 0.5f * (top + bottom);
      const float scaleX = portHudInvScale * portWiden;
      CGraphics::SetOrtho(midX + (left - midX) * scaleX, midX + (right - midX) * scaleX,
                          midY + (top - midY) * portHudInvScale,
                          midY + (bottom - midY) * portHudInvScale,
                          mCameraParms.mOrthographic.mNear, mCameraParms.mOrthographic.mFar);
    } else {
      CGraphics::SetOrtho(mCameraParms.mOrthographic.mLeft, mCameraParms.mOrthographic.mRight,
                          mCameraParms.mOrthographic.mTop, mCameraParms.mOrthographic.mBottom,
                          mCameraParms.mOrthographic.mNear, mCameraParms.mOrthographic.mFar);
    }
#else
    CGraphics::SetOrtho(mCameraParms.mOrthographic.mLeft, mCameraParms.mOrthographic.mRight,
                        mCameraParms.mOrthographic.mTop, mCameraParms.mOrthographic.mBottom,
                        mCameraParms.mOrthographic.mNear, mCameraParms.mOrthographic.mFar);
#endif
  }

  CGraphics::SetViewPointMatrix(CTransform4f::Translate(parms.GetCameraOffset()) *
                                GetWorldTransform());
  CGuiWidget::Draw(parms);
}

CVector3f CGuiCamera::ConvertToScreenSpace(const CVector3f& point) const {
  CVector3f rotated = RotateTranslateW2O(point);

  if (rotated.IsNonZero() && mProjection == kProjection_Perspective) {
    CMatrix4f xf = CGraphics::CalculatePerspectiveMatrix(
        mCameraParms.mPerspective.mFov, mCameraParms.mPerspective.mAspect,
        mCameraParms.mPerspective.mNear, mCameraParms.mPerspective.mFar);

    return xf.MultiplyOneOverW(rotated);
  }

  return CVector3f(-1.f, -1.f, 1.f);
}
