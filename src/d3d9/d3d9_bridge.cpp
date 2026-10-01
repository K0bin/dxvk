
#include "d3d9_device.h"
#include "d3d9_interface.h"
#include "d3d9_bridge.h"
#include "d3d9_swapchain.h"
#include "d3d9_surface.h"
#include "d3d9_format.h"

namespace dxvk {

  DxvkLegacyD3DDeviceBridge::DxvkLegacyD3DDeviceBridge(D3D9DeviceEx* pDevice)
    : m_device(pDevice) {
  }

  DxvkLegacyD3DDeviceBridge::~DxvkLegacyD3DDeviceBridge() {
  }

  ULONG STDMETHODCALLTYPE DxvkLegacyD3DDeviceBridge::AddRef() {
    return m_device->AddRef();
  }

  ULONG STDMETHODCALLTYPE DxvkLegacyD3DDeviceBridge::Release() {
    return m_device->Release();
  }

  HRESULT STDMETHODCALLTYPE DxvkLegacyD3DDeviceBridge::QueryInterface(
          REFIID  riid,
          void** ppvObject) {
    return m_device->QueryInterface(riid, ppvObject);
  }

  HRESULT DxvkLegacyD3DDeviceBridge::UpdateTextureFromBuffer(
        IDirect3DSurface9*  pDestSurface,
        IDirect3DSurface9*  pSrcSurface,
        const RECT*         pSrcRect,
        const POINT*        pDestPoint) {
    auto lock = m_device->LockDevice();

    D3D9Surface* dst = static_cast<D3D9Surface*>(pDestSurface);
    D3D9Surface* src = static_cast<D3D9Surface*>(pSrcSurface);

    if (unlikely(dst == nullptr || src == nullptr))
      return D3DERR_INVALIDCALL;

    // CopyRects will not pass a null pSrcRect, but check anyway
    if (unlikely(pSrcRect == nullptr))
      return D3DERR_INVALIDCALL;

    // validate dimensions to ensure we calculate a meaningful srcOffset & extent
    if (unlikely(pSrcRect->left < 0
              || pSrcRect->top  < 0
              || pSrcRect->right  <= pSrcRect->left
              || pSrcRect->bottom <= pSrcRect->top))
      return D3DERR_INVALIDCALL;

    // CopyRects will not pass a null pDestPoint, but check anyway
    if (unlikely(pDestPoint == nullptr))
      return D3DERR_INVALIDCALL;

    // validate dimensions to ensure we caculate a meaningful dstOffset
    if (unlikely(pDestPoint->x < 0
              || pDestPoint->y < 0))
      return D3DERR_INVALIDCALL;

    D3D9CommonTexture* srcTextureInfo = src->GetCommonTexture();
    D3D9CommonTexture* dstTextureInfo = dst->GetCommonTexture();

    VkOffset3D srcOffset = { pSrcRect->left,
                             pSrcRect->top,
                             0u };

    VkExtent3D extent = { uint32_t(pSrcRect->right - pSrcRect->left), uint32_t(pSrcRect->bottom - pSrcRect->top), 1 };

    VkOffset3D dstOffset = { pDestPoint->x,
                             pDestPoint->y,
                             0u };

    m_device->UpdateTextureFromBuffer(
      srcTextureInfo, dstTextureInfo,
      src->GetSubresource(), dst->GetSubresource(),
      srcOffset, extent, dstOffset
    );

    dstTextureInfo->SetNeedsReadback(dst->GetSubresource(), true);

    if (dstTextureInfo->IsAutomaticMip())
      m_device->MarkTextureMipsDirty(dstTextureInfo);

    return D3D_OK;
  }


  /**
   * \brief D3D8 CopyRects implementation
   */
  HRESULT STDMETHODCALLTYPE DxvkLegacyD3DDeviceBridge::CopyRects(
          IDirect3DSurface9*  pSourceSurface,
    const RECT*               pSourceRectsArray,
          UINT                cRects,
          IDirect3DSurface9*  pDestinationSurface,
    const POINT*              pDestPointsArray) {
    D3D9DeviceLock lock = m_device->LockDevice();

    // The source and destination surfaces can not be identical.
    if (unlikely(pSourceSurface == nullptr ||
                 pDestinationSurface == nullptr ||
                 pSourceSurface == pDestinationSurface)) {
      return D3DERR_INVALIDCALL;
    }

    // TODO: No stretching or clipping of either source or destination rectangles.
    // All src/dest rectangles must fit within the dest surface.

    Com<D3D9Surface> src = static_cast<D3D9Surface*>(pSourceSurface);
    Com<D3D9Surface> dst = static_cast<D3D9Surface*>(pDestinationSurface);

    D3D9CommonTexture* srcTex = src->GetCommonTexture();
    D3D9CommonTexture* dstTex = dst->GetCommonTexture();

    // This method does not support format conversion.
    if (unlikely(srcTex->Desc()->Format != dstTex->Desc()->Format))
      return D3DERR_INVALIDCALL;

    // This method cannot be applied to surfaces whose formats are classified as depth stencil formats.
    if (unlikely(IsDepthStencilFormat(dstTex->Desc()->Format)))
      return D3DERR_INVALIDCALL;

    if (unlikely(dstTex->GetFormatMapping().ConversionFormatInfo.FormatType != D3D9ConversionFormat_None && (srcTex->GetImage() == nullptr || dstTex->GetImage() == nullptr))) {
      Logger::err("CopyRects with formats that need conversion is only supported if it's an image to image copy.");
      return D3DERR_NOTAVAILABLE;
    }

    // If pSourceRectsArray is NULL, then the entire surface is copied
    RECT rect;
    POINT point = { 0, 0 };
    if (pSourceRectsArray == NULL) {
      cRects = 1;
      rect.top    = rect.left = 0;
      rect.right  = srcTex->Desc()->Width;
      rect.bottom = srcTex->Desc()->Height;
      pSourceRectsArray = &rect;

      pDestPointsArray = &point;
    }

    bool srcHasImage = srcTex->GetImage() != nullptr;
    bool dstHasImage = srcTex->GetImage() != nullptr;

    // Flush remaining managed texture uploads.
    if (srcHasImage && srcTex->NeedsUpload(src->GetSubresource()))
      m_device->FlushImage(srcTex, src->GetSubresource());
    if (dstHasImage && dstTex->NeedsUpload(dst->GetSubresource()))
      m_device->FlushImage(dstTex, dst->GetSubresource());

    for (uint32_t i = 0; i < cRects; i++) {
      RECT srcRect;
      POINT dstPoint;
      srcRect = pSourceRectsArray[i];
      dstPoint = pDestPointsArray[i];

      VkOffset2D dstOffset = { dstPoint.x, dstPoint.y };
      VkOffset2D srcOffset = { srcRect.left, srcRect.top };
      VkExtent2D extent = { uint32_t(srcRect.right - srcRect.left), uint32_t(srcRect.bottom - srcRect.top) };

      VkExtent3D srcTexLevelExtent = srcTex->GetExtentMip(src->GetMipLevel());
      VkExtent3D dstTexLevelExtent = dstTex->GetExtentMip(dst->GetMipLevel());

      VkExtent3D srcExtent = { extent.width, extent.height, 1 };
      VkExtent3D dstExtent = { extent.width, extent.height, 1 };

      auto formatInfo = lookupFormatInfo(dstTex->GetFormatMapping().Format);
      VkOffset3D alignedDstOffset = {
        int32_t(alignDown(dstOffset.x, formatInfo->blockSize.width)),
        int32_t(alignDown(dstOffset.y, formatInfo->blockSize.height)),
        0
      };
      VkOffset3D alignedSrcOffset = {
        int32_t(alignDown(srcOffset.x, formatInfo->blockSize.width)),
        int32_t(alignDown(srcOffset.y, formatInfo->blockSize.height)),
        0
      };

      srcExtent.width += dstOffset.x - alignedDstOffset.x;
      srcExtent.height += dstOffset.y - alignedDstOffset.y;
      dstExtent.width += dstOffset.x - alignedDstOffset.x;
      dstExtent.height += dstOffset.y - alignedDstOffset.y;
      extent.width = std::min(srcExtent.width, dstExtent.width);
      extent.height = std::min(srcExtent.height, dstExtent.height);

      VkExtent3D extentBlockCount = util::computeBlockCount(VkExtent3D { extent.width, extent.height, formatInfo->blockSize.depth }, formatInfo->blockSize);
      VkExtent3D alignedExtent = util::computeBlockExtent(extentBlockCount, formatInfo->blockSize);

      alignedExtent = util::snapExtent3D(alignedDstOffset, alignedExtent, dstTexLevelExtent);
      alignedExtent = util::snapExtent3D(alignedSrcOffset, alignedExtent, srcTexLevelExtent);

      VkOffset3D srcOffsetBlockCount = util::computeBlockOffset(alignedSrcOffset, formatInfo->blockSize);
      VkOffset3D dstOffsetBlockCount = util::computeBlockOffset(alignedSrcOffset, formatInfo->blockSize);
      VkExtent3D srcTexLevelExtentBlockCount = util::computeBlockCount(srcTexLevelExtent, formatInfo->blockSize);
      VkExtent3D dstTexLevelExtentBlockCount = util::computeBlockCount(dstTexLevelExtent, formatInfo->blockSize);
      VkDeviceSize srcPitch = align(srcTexLevelExtentBlockCount.width * formatInfo->elementSize, 4);
      VkDeviceSize dstPitch = align(dstTexLevelExtentBlockCount.width * formatInfo->elementSize, 4);
      VkDeviceSize copySrcOffset = srcOffsetBlockCount.z * srcTexLevelExtentBlockCount.height * srcPitch
          + srcOffsetBlockCount.y * srcPitch
      + srcOffsetBlockCount.x * formatInfo->elementSize;
      VkDeviceSize copyDstOffset = dstOffsetBlockCount.z * dstTexLevelExtentBlockCount.height * dstPitch
          + dstOffsetBlockCount.y * dstPitch
          + dstOffsetBlockCount.x * formatInfo->elementSize;

      if (srcHasImage && dstHasImage) {
        // The backend will thankfully handle all VkUsage issues.
        m_device->EmitCs([cDstImage = dstTex->GetImage(),
          cSrcImage = srcTex->GetImage(),
          cSrcOffset = alignedSrcOffset,
          cDstOffset = alignedDstOffset,
          cDstLayers = vk::makeSubresourceLayers(dstTex->GetSubresourceFromIndex(
            formatInfo->aspectMask, dst->GetSubresource())),
          cSrcLayers = vk::makeSubresourceLayers(srcTex->GetSubresourceFromIndex(
            formatInfo->aspectMask, src->GetSubresource())),
          cExtent = alignedExtent] (DxvkContext* ctx) {
          ctx->copyImage(cDstImage, cDstLayers, cDstOffset,
            cSrcImage, cSrcLayers, cSrcOffset, cExtent);
        });
      }
      else if (!srcHasImage && dstHasImage) {
        // Just use the staging buffer upload here.
        m_device->UpdateTextureFromBuffer(dstTex, srcTex, src->GetSubresource(), dst->GetSubresource(), alignedSrcOffset, alignedExtent, alignedDstOffset);
      }
      else if (srcHasImage && !dstHasImage) {
        m_device->EmitCs([cDstBuffer = dstTex->GetBuffer(),
        cSrcImage = srcTex->GetImage(),
        cDstLayers = vk::makeSubresourceLayers(dstTex->GetSubresourceFromIndex(
          formatInfo->aspectMask, dst->GetSubresource())),
        cSrcLayers = vk::makeSubresourceLayers(srcTex->GetSubresourceFromIndex(
          formatInfo->aspectMask, src->GetSubresource())),
        cSrcOffset = alignedSrcOffset,
        cBufferOffset = copyDstOffset,
        cDstPitch = dstPitch,
        cExtent = alignedExtent
          ] (DxvkContext* ctx) {
          ctx->copyImageToBuffer(cDstBuffer, cBufferOffset, cDstPitch, 0, VK_FORMAT_UNDEFINED, cSrcImage, cSrcLayers, cSrcOffset, cExtent);
        });
      }
      else if (!srcHasImage && !dstHasImage) {
        // Make sure both have buffers.
        // This copies the data from the memory mapped file to a Vulkan buffer if necessary.
        dstTex->CreateBuffer(true, dstTex->GetTotalSize());
        srcTex->CreateBuffer(true, srcTex->GetTotalSize());
        const Rc<DxvkBuffer>& srcBuffer = srcTex->GetBuffer();
        const Rc<DxvkBuffer>& dstBuffer = dstTex->GetBuffer();

        if (srcTex->NeedsReadback(src->GetSubresource())) {
          Logger::warn("Stalling in CopyRects because of src.");
          m_device->WaitForResource(*srcBuffer, srcTex->GetMappingBufferSequenceNumber(src->GetSubresource()), 0);
          srcTex->SetNeedsReadback(src->GetSubresource(), false);
        }
        if (srcTex->NeedsReadback(src->GetSubresource())) {
          Logger::warn("Stalling in CopyRects because of dst.");
          m_device->WaitForResource(*dstBuffer, dstTex->GetMappingBufferSequenceNumber(dst->GetSubresource()), 0);
          dstTex->SetNeedsReadback(dst->GetSubresource(), false);
        }

        // We assume that using CopyRects with DEFAULT->SYSTEMMEM and then SYSTEMMEM->SYSTEMMEM doesn't really happen in practice.
        // So the buffers don't need sync and we can just do the copy on the CPU here.

        const void* srcMapPtr = srcBuffer->mapPtr(copySrcOffset);
        void* dstMapPtr = dstBuffer->mapPtr(copyDstOffset);
        VkDeviceSize dirtySize = extentBlockCount.width * extentBlockCount.height * extentBlockCount.depth * formatInfo->elementSize;
        D3D9BufferSlice slice = m_device->AllocStagingBuffer(dirtySize);
        util::packImageData(
          dstMapPtr, srcMapPtr, srcPitch, 0, dstPitch, 0, VK_IMAGE_TYPE_2D, alignedExtent, 1, formatInfo,
          VK_IMAGE_ASPECT_COLOR_BIT);
      }

      dstTex->SetNeedsReadback(dst->GetSubresource(), true);
    }

    return D3D_OK;
  }


  bool DxvkLegacyD3DDeviceBridge::IsSupportedSurfaceFormat(D3DFORMAT Format) {
    auto mapping = m_device->LookupFormat(EnumerateFormat(Format));
    return mapping.IsValid();
  }

  DxvkLegacyD3DInterfaceBridge::DxvkLegacyD3DInterfaceBridge(D3D9InterfaceEx* pObject)
    : m_interface(pObject) {
  }

  DxvkLegacyD3DInterfaceBridge::~DxvkLegacyD3DInterfaceBridge() {
  }

  ULONG STDMETHODCALLTYPE DxvkLegacyD3DInterfaceBridge::AddRef() {
    return m_interface->AddRef();
  }

  ULONG STDMETHODCALLTYPE DxvkLegacyD3DInterfaceBridge::Release() {
    return m_interface->Release();
  }

  HRESULT STDMETHODCALLTYPE DxvkLegacyD3DInterfaceBridge::QueryInterface(
          REFIID  riid,
          void** ppvObject) {
    return m_interface->QueryInterface(riid, ppvObject);
  }

  void DxvkLegacyD3DInterfaceBridge::SetD3DCompatibility(D3DCompatibility d3dCompatibility) const {
    // The D3D9Ex compatibility flag is internal only, and can't be set by the bridge
    if (likely(d3dCompatibility != D3DCompatibility::D3D9Ex)) {
      m_interface->SetD3DCompatibility(d3dCompatibility);
    } else {
      Logger::err("DxvkLegacyD3DInterfaceBridge::SetD3DCompatibility: Invalid compatibility level: D3D9Ex");
    }
  }

  const Config* DxvkLegacyD3DInterfaceBridge::GetConfig() const {
    return &m_interface->GetInstance()->config();
  }

}
