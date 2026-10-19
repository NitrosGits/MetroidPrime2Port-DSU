#include "GuiSys/CGuiFrame.hpp"

#include "GuiSys/CGuiCamera.hpp"
#include "GuiSys/CGuiFactories.hpp"
#include "GuiSys/CGuiFrameLoader.hpp"
#include "GuiSys/CGuiFrameModelDatabase.hpp"
#include "GuiSys/CGuiHeadWidget.hpp"
#include "GuiSys/CGuiLight.hpp"
#include "GuiSys/CGuiModel.hpp"
#include "GuiSys/CGuiWidget.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/Streams/CMemoryInStream.hpp"
#include "rstl/algorithm.hpp"
#include "rstl/list.hpp"

#ifdef TARGET_PC
#include "Kyoto/Math/CAABox.hpp"

// Port: widescreen HUD, as MP1's CGuiFrame::Draw. The camera widened the projection by `spread`
// (CGuiCamera::Draw); each widget then moves out about the view axis by that factor at its own
// anchor (a model's bounds centre, since several icons keep their place inside the vertices and
// their widget at the frame centre), as a rigid slide in the view plane, so its shape and depth are
// untouched. A model spanning both outer quarters of the authored view (the visor frame, the struts)
// is stretched instead, so it still meets the wide view's edges.
static void PortDrawSpread(const CGuiCamera& camera, const CGuiWidget* widget,
                           const CGuiWidgetDrawParms& parms) {
  const float spread = camera.GetAspectSpread();
  CTransform4f savedXf = CTransform4f::Identity();
  bool savedValid = false;
  if (spread == 1.f || widget->GetParent() == nullptr) {
    widget->Draw(parms);
    return;
  }
  const CTransform4f view = camera.GetSpreadView();
  const CTransform4f invView = view.GetInverse();
  const CTransform4f world = widget->GetWorldTransform();
  CVector3f anchor = world.GetTranslation();
  bool stretch = false;
  CAABox bounds(CVector3f(0.f, 0.f, 0.f), CVector3f(0.f, 0.f, 0.f));
  if (widget->GetWidgetTypeID() == 'MODL' &&
      static_cast< const CGuiModel* >(widget)->PortGetBounds(bounds)) {
    anchor = world * bounds.GetCenterPoint();
    const CAABox eyeBounds = bounds.GetTransformedAABox(invView * world);
    float quarterWidth = 0.5f * camera.GetSpreadHalfWidth();
    if (camera.IsPerspective()) {
      quarterWidth *= eyeBounds.GetMaxPoint().GetY();
    }
    const float centerX = camera.GetSpreadCenterX();
    stretch = quarterWidth > 0.f && eyeBounds.GetMinPoint().GetX() < centerX - quarterWidth &&
              eyeBounds.GetMaxPoint().GetX() > centerX + quarterWidth;
  }
  CTransform4f moved = world;
  if (stretch) {
    moved = view * CTransform4f::Translate(camera.GetSpreadCenterX(), 0.f, 0.f) *
            CTransform4f::Scale(spread, 1.f, 1.f) *
            CTransform4f::Translate(-camera.GetSpreadCenterX(), 0.f, 0.f) * invView * world;
  } else {
    const CVector3f eye = invView * anchor;
    moved = CTransform4f::Translate(view.Rotate(
                CVector3f((spread - 1.f) * (eye.GetX() - camera.GetSpreadCenterX()), 0.f, 0.f))) *
            world;
  }
  if (widget->PortSetWorldTransform(moved, savedXf, savedValid)) {
    widget->Draw(parms);
    widget->PortRestoreWorldTransform(savedXf, savedValid);
  } else {
    widget->Draw(parms);
  }
}
#endif
namespace rstl {
class CWidgetFartherFromCamera {
public:
  CWidgetFartherFromCamera() {}
  bool operator()(const CGuiWidget* a, const CGuiWidget* b) const {
    return a->GetWorldPosition().GetY() > b->GetWorldPosition().GetY();
  }
};
} // namespace rstl

uint CGuiFrame::ReadVersion(CInputStream& in) { return in.Get< uint >(); }

rstl::vector< CToken > CGuiFrame::LoadAssets(CInputStream& in, CSimplePool* pool, uint version) {
  rstl::vector< CToken > assets;
  if (version > 1u) {
    assets.reserve(in.ReadInt32());
    for (int i = 0; i < assets.capacity(); ++i) {
      const FourCC type = in.Get< uint >();
      const CAssetId id = in.Get< uint >();
      CToken token = pool->GetObj(SObjectTag(type, id));
      token.Lock();
      assets.push_back_unsafe(token);
    }
  } else {
    in.ReadInt32();
    in.ReadInt32();
    in.ReadInt32();
  }
  return assets;
}

CGuiFrame::CGuiFrame(CInputStream& in, CSimplePool* pool)
: mVersion(ReadVersion(in))
, mAssets(LoadAssets(in, pool, mVersion))
, mRootWidget(nullptr)
, mCamera(nullptr)
, mLights(rstl::vector< CGuiLight* >(8, static_cast< CGuiLight* >(nullptr)))
, mModelDatabase(rs_new CGuiFrameModelDatabase(in, pool))
, mLoaded(false) {
  LoadWidgetsInGame(in, pool, mVersion);
}

CGuiFrame::~CGuiFrame() { delete mRootWidget; }

void CGuiFrame::LoadWidgetsInGame(CInputStream& in, CSimplePool* pool, uint version) {
  const int count = in.Get< int >();
  mWidgets.reserve(count);
  mWidgetIds.Reserve(count);

  int drawCount = 0;
  int updateCount = 0;
  int inputCount = 0;
  int preDrawCount = 0;
  for (int i = 0; i < count; ++i) {
    CGuiWidget* widget = FGuiWidgetFactoryInGame(in.Get< uint >(), this, in, pool, version);
    if (widget->GetWidgetTypeID() != 'CAMR' && widget->GetWidgetTypeID() != 'LITE') {
      mWidgets.push_back_unsafe(widget);
      const CGuiWidget::EWidgetUsageFlags flags = widget->GetWidgetUsageFlags();
      if (flags & CGuiWidget::kWUF_Draw) {
        ++drawCount;
      }
      if (flags & CGuiWidget::kWUF_Update) {
        ++updateCount;
      }
      if (flags & CGuiWidget::kWUF_Input) {
        ++inputCount;
      }
      if (flags & CGuiWidget::kWUF_PreDraw) {
        ++preDrawCount;
      }
    }
  }

  mDrawWidgets.reserve(drawCount);
  mUpdateWidgets.reserve(updateCount);
  mInputWidgets.reserve(inputCount);
  mPreDrawWidgets.reserve(preDrawCount);
  for (rstl::vector< CGuiWidget* >::const_iterator it = mWidgets.begin(); it != mWidgets.end();
       ++it) {
    CGuiWidget* widget = *it;
    const CGuiWidget::EWidgetUsageFlags flags = widget->GetWidgetUsageFlags();
    if (flags & CGuiWidget::kWUF_Draw) {
      mDrawWidgets.push_back_unsafe(widget);
    }
    if (flags & CGuiWidget::kWUF_Update) {
      mUpdateWidgets.push_back_unsafe(widget);
    }
    if (flags & CGuiWidget::kWUF_Input) {
      mInputWidgets.push_back_unsafe(widget);
    }
    if (flags & CGuiWidget::kWUF_PreDraw) {
      mPreDrawWidgets.push_back_unsafe(widget);
    }
  }
  Initialize();
}

void CGuiFrame::Initialize() {
  SortDrawOrder();
  CGuiWidget* root = mRootWidget;
  root->RecalcWidgetColor(kTM_ChildrenAndSiblings);
  root->DispatchInitialize();
}

void CGuiFrame::Draw(const CGuiWidgetDrawParms& parms) const {
  CGraphics::SetCullMode(kCM_None);
  CGraphics::ResetGfxStates();
  CGraphics::SetAmbientColor(CColor::White());
  DisableLights();
  mCamera->Draw(parms);
  CGraphics::SetTevOp(kTS_Stage0, CGraphics::kEnvModulate);
  CGraphics::SetBlendMode(kBM_Blend, kBF_SrcAlpha, kBF_InvSrcAlpha, kLO_Clear);
  ApplyLights();

  CGuiModel::BeginDraw();
  for (rstl::vector< CGuiWidget* >::const_iterator it = mPreDrawWidgets.begin();
       it != mPreDrawWidgets.end(); ++it) {
    CGuiWidget* widget = *it;
    if (widget->GetIsVisible()) {
#ifdef TARGET_PC
      PortDrawSpread(*mCamera, widget, parms);
#else
      widget->Draw(parms);
#endif
    }
  }
  CGuiModel::EndDraw();
  DisableLights();

  for (rstl::vector< CGuiWidget* >::const_iterator it = mDrawWidgets.begin();
       it != mDrawWidgets.end(); ++it) {
    CGuiWidget* widget = *it;
    if (widget->GetIsVisible()) {
#ifdef TARGET_PC
      PortDrawSpread(*mCamera, widget, parms);
#else
      widget->Draw(parms);
#endif
    }
  }
  CGraphics::SetCullMode(kCM_Front);
  CGraphics::SetDepthWriteMode(true, kE_LEqual, true);
}

void CGuiFrame::Update(float dt) {
  for (rstl::vector< CGuiWidget* >::iterator it = mUpdateWidgets.begin();
       it != mUpdateWidgets.end(); ++it) {
    (*it)->Update(dt);
  }

  mActiveLights.clear();
  mAmbientColor = CColor::Black();
  // The original compares against size() after clear(), not the fixed capacity.
  for (int i = 0; i < mLights.size() && i < mActiveLights.size(); ++i) {
    bool added = false;
    CGuiLight* light = mLights[i];
    if (light != nullptr && light->GetIsVisible()) {
      const CColor& color = light->GetModifiedColor();
      if (color.GetRedu8() != 0 || color.GetGreenu8() != 0 || color.GetBlueu8() != 0) {
        mActiveLights.push_back(light->BuildLight());
        added = true;
      }
      mAmbientColor = CColor::Add(mAmbientColor, light->GetAmbientContribution());
    }
    if (!added) {
      mActiveLights.push_back(CLight::BuildPoint(CVector3f::Zero(), CColor::Black()));
    }
  }
}

void CGuiFrame::ProcessUserInput(const CFinalInput& input) {
  rstl::list< CGuiWidget* > activeWidgets;
  for (rstl::vector< CGuiWidget* >::const_iterator it = mInputWidgets.begin();
       it != mInputWidgets.end(); ++it) {
    CGuiWidget* widget = *it;
    if (widget->GetIsActive()) {
      activeWidgets.push_back(widget);
    }
  }
  for (rstl::list< CGuiWidget* >::iterator it = activeWidgets.begin(); it != activeWidgets.end();
       ++it) {
    (*it)->ProcessUserInput(input);
  }
}

bool CGuiFrame::GetIsFinishedLoading() const {
  if (mLoaded) {
    return true;
  }
  for (rstl::vector< CToken >::const_iterator it = mAssets.begin(); it != mAssets.end(); ++it) {
    if (!it->IsLoaded()) {
      return false;
    }
  }
  mLoaded = true;
  return true;
}

void CGuiFrame::AddLight(CGuiLight* light) { mLights[light->GetLightIndex()] = light; }

void CGuiFrame::RemoveLight(CGuiLight* light) {
  if (mLights[light->GetLightIndex()] == light) {
    mLights[light->GetLightIndex()] = nullptr;
  }
}

void CGuiFrame::DisableLights() const { EnableLights(0); }

void CGuiFrame::ApplyLights() const {
  for (int i = 0; i < mActiveLights.size(); ++i) {
    CGraphics::LoadLight(static_cast< ERglLight >(i), mActiveLights[i]);
  }
}

void CGuiFrame::EnableLights(uint mask) const {
  uint enabled = 0;
  for (int i = 0; i < mLights.size(); ++i) {
    if ((mask & (1 << i)) != 0) {
      CGuiLight* light = mLights[i];
      if (light != nullptr && light->GetIsVisible()) {
        enabled |= 1 << i;
      }
    }
  }
  if (enabled == 0) {
    CGraphics::DisableAllLights();
    CGraphics::SetAmbientColor(CColor::White());
  } else {
    CGraphics::SetLightState(enabled);
    CGraphics::SetAmbientColor(mAmbientColor);
  }
}

void CGuiFrame::SortDrawOrder() {
  rstl::sort(mDrawWidgets.begin(), mDrawWidgets.end(), rstl::CWidgetFartherFromCamera());
  rstl::sort(mPreDrawWidgets.begin(), mPreDrawWidgets.end(), rstl::CWidgetFartherFromCamera());
}

CGuiWidget* CGuiFrame::FindWidget(const rstl::string& name) const {
  const short id = mWidgetIds.FindWidgetID(name);
  return id != CGuiWidgetIdDB::kInvalidWidgetId ? FindWidget(id) : nullptr;
}

CGuiWidget* CGuiFrame::FindWidget(short id) const {
  return mRootWidget != nullptr ? mRootWidget->FindWidget(id) : nullptr;
}

void CGuiFrame::SetHeadWidget(CGuiHeadWidget* widget) { mRootWidget = widget; }

void CGuiFrame::SetFrameCamera(CGuiCamera* camera) { mCamera = camera; }

CGuiWidget* CGuiFrame::FindWidget(const char* name) const {
  return FindWidget(rstl::string_l(name));
}

CGuiFrameLoader::CGuiFrameLoader(CAssetId asset, CResFactory& factory, CSimplePool& pool)
: mTag('FRME', asset)
, mPool(&pool)
, mBufferLength(factory.ResourceSize(mTag))
, mBuffer(static_cast< uchar* >(CMemory::Alloc(mBufferLength, IAllocator::kHI_RoundUpLen)))
, mRequest(
      factory.GetResLoader().LoadResourceAsync(mTag, reinterpret_cast< char* >(mBuffer.get()))) {}

CGuiFrameLoader::~CGuiFrameLoader() {}

bool CGuiFrameLoader::IsFinishedLoading() const { return mRequest->IsComplete(); }

CGuiFrame* CGuiFrameLoader::CreateFrame() {
  if (mRequest->IsComplete()) {
    CMemoryInStream in(mBuffer.get(), mBufferLength);
    return rs_new CGuiFrame(in, mPool);
  }
  return nullptr;
}
