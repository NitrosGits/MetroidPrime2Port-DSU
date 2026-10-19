#ifndef _CGUIMODEL
#define _CGUIMODEL

#include "GuiSys/CGuiWidget.hpp"

class CModelFlags;
#ifdef TARGET_PC
class CAABox;
#endif

class CGuiModel : public CGuiWidget {
public:
  CGuiModel(const CGuiWidgetParms& parms, CAssetId modelId, uint lightMask, int modelIndex);

  // CGuiObject
  ~CGuiModel() override;

  // CGuiWidget
  FourCC GetWidgetTypeID() const override;
  EWidgetUsageFlags GetWidgetUsageFlags() const override;
  void Draw(const CGuiWidgetDrawParms& parms) const override;

  void DrawModel(const CModelFlags& flags) const; // Guessed name
#ifdef TARGET_PC
  // Port: the model's local bounds, for the widescreen HUD spread. False until the frame has loaded.
  bool PortGetBounds(CAABox& out) const;
#endif
  static void BeginDraw();                        // Guessed name
  static void EndDraw();                          // Guessed name
  static CGuiModel* Create(CGuiFrame* frame, CInputStream& in, uint version);

private:
  static void UpdateDrawState(int drawFlags, CColor color); // Guessed name

  CAssetId mModelId;
  int mModelIndex;
  uint mLightMask;
};
CHECK_SIZEOF(CGuiModel, 0xc8)

#endif // _CGUIMODEL
