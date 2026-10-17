#include "GuiSys/CGuiFrameModelDatabase.hpp"

#include "Kyoto/Alloc/CMemory.hpp"
#include "Kyoto/Basics/CBasics.hpp"
#include "Kyoto/CFrameDelayedKiller.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Graphics/CCubeModel.hpp"
#include "Kyoto/Graphics/CModel.hpp"
#include "Kyoto/Streams/CInputStream.hpp"

#include <dolphin/os.h>
#ifdef TARGET_PC
#include "Kyoto/Graphics/PortDrawDebug.hpp"
#endif

// Guessed local helper name; serialized lengths determine the section boundaries.
static uchar* MemoryFromPartData(uchar*& dataCur, int*& sectionSizeCur) {
  uchar* result;
  if (*sectionSizeCur != 0) {
    result = dataCur;
    dataCur += *sectionSizeCur;
  } else {
    result = nullptr;
  }
  ++sectionSizeCur;
  return result;
}

CGuiFrameModelDatabase::CGuiFrameModelDatabase(CInputStream& in, CSimplePool* pool)
: mBufferSize(in.ReadInt32())
, mBuffer(mBufferSize != 0
              ? static_cast< uchar* >(CMemory::Alloc(mBufferSize, IAllocator::kHI_RoundUpLen))
              : nullptr)
, mModels(in.ReadInt32(), rstl::auto_ptr< CCubeModel >())
, mSurfaces(mModels.size(), rstl::vector< void* >()) {
  CModel::AddToTotal(mBufferSize);
  rstl::vector< int > sectionSizes(in);
  if (!sectionSizes.empty()) {
    in.Get(mBuffer.get(), mBufferSize);

    int* sectionSizeCur = sectionSizes.data();
    const int modelCount = mModels.size();
    uchar* dataCur = mBuffer.get();
    const void* materialData = MemoryFromPartData(dataCur, sectionSizeCur);
    CCubeModel::MakeTexturesFromMats(materialData, mTextures, *pool, true);

    for (int i = 0; i < modelCount; ++i) {
#ifdef TARGET_PC
      // Port: like CModel, pass section sizes (the instance converts the vertex data to host
      // order) and convert each disc surface header to the native layout.
      // Port: the FRME display lists run in vertex format 1 (S16 x3 normals, GXSetVtxAttrFmt in
      // CGraphics), i.e. 6-byte packed normals; the visor flag is what makes SetArraysCurrent use that
      // stride and normal count (the host indexes by the real byte size, retail never checked).
      const uchar kPackedNormals = 1;
      const uint positionsSize = *sectionSizeCur;
      const void* positions = MemoryFromPartData(dataCur, sectionSizeCur);
      const uint normalsSize = *sectionSizeCur;
      const void* normals = MemoryFromPartData(dataCur, sectionSizeCur);
      const uint colorsSize = *sectionSizeCur;
      const void* colors = MemoryFromPartData(dataCur, sectionSizeCur);
      const uint uvsSize = *sectionSizeCur;
      const void* uvs = MemoryFromPartData(dataCur, sectionSizeCur);
      const uint packedUvsSize = *sectionSizeCur;
      const void* packedUvs = MemoryFromPartData(dataCur, sectionSizeCur);
      const uint surfaceCount = CBasics::SwapBytes(
          *reinterpret_cast< const uint* >(MemoryFromPartData(dataCur, sectionSizeCur)));
      rstl::vector< void* >& surfaces = mSurfaces[i];
      surfaces.reserve(surfaceCount);
      for (uint j = 0; j < surfaceCount; ++j) {
        uchar* surface = MemoryFromPartData(dataCur, sectionSizeCur);
        CCubeSurface::ConvertSurfaceHeader(surface);
        surfaces.push_back_unsafe(surface);
      }

      const rstl::auto_ptr< CCubeModel > model(rs_new CCubeModel(
          &surfaces, &mTextures, materialData, positions, normals, colors, uvs, packedUvs,
          CAABox::Identity(), kPackedNormals, true, i, positionsSize, normalsSize, colorsSize,
          uvsSize, packedUvsSize));
      PortDrawDebug::LabelGuiFrame(model.get(), i);
      mModels[i] = model;
#else
      const void* positions = MemoryFromPartData(dataCur, sectionSizeCur);
      const void* normals = MemoryFromPartData(dataCur, sectionSizeCur);
      const void* colors = MemoryFromPartData(dataCur, sectionSizeCur);
      const void* uvs = MemoryFromPartData(dataCur, sectionSizeCur);
      const void* packedUvs = MemoryFromPartData(dataCur, sectionSizeCur);
      const uint surfaceCount = CBasics::SwapBytes(
          *reinterpret_cast< const uint* >(MemoryFromPartData(dataCur, sectionSizeCur)));
      rstl::vector< void* >& surfaces = mSurfaces[i];
      surfaces.reserve(surfaceCount);
      for (uint j = 0; j < surfaceCount; ++j) {
        surfaces.push_back_unsafe(MemoryFromPartData(dataCur, sectionSizeCur));
      }

      const rstl::auto_ptr< CCubeModel > model(rs_new CCubeModel(
          &surfaces, &mTextures, materialData, positions, normals, colors, uvs, packedUvs,
          CAABox::Identity(), 0, true, i));
      mModels[i] = model;
#endif
    }
  }

  DCFlushRange(mBuffer.get(), mBufferSize);
}

CGuiFrameModelDatabase::~CGuiFrameModelDatabase() {
  CModel::RemoveFromTotal(mBufferSize);
  CFrameDelayedKiller::ScheduleDeletion(CFrameDelayedKiller::kWhichFrame_NextFrame,
                                       mBuffer.release());
}

void CGuiFrameModelDatabase::Draw(int index, const CModelFlags& flags) const {
  mModels[index]->Draw(flags);
}

const CCubeModel* CGuiFrameModelDatabase::GetModel(int index) const {
  return mModels[index].get();
}
