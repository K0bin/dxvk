
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


  /**
   * \brief D3D8 CopyRects implementation
   */
  HRESULT DxvkLegacyD3DDeviceBridge::CopyRects(
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
      Logger::err("CopyRects with formats, that need conversion, is only supported if it's an image to image copy.");
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
    bool dstHasImage = dstTex->GetImage() != nullptr;

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

      VkExtent3D srcExtent = { extent.width, extent.height, 1 };
      srcExtent.width += dstOffset.x - alignedDstOffset.x;
      srcExtent.height += dstOffset.y - alignedDstOffset.y;
      VkExtent3D dstExtent = { extent.width, extent.height, 1 };
      dstExtent.width += dstOffset.x - alignedDstOffset.x;
      dstExtent.height += dstOffset.y - alignedDstOffset.y;
      extent.width = std::min(srcExtent.width, dstExtent.width);
      extent.height = std::min(srcExtent.height, dstExtent.height);

      VkExtent3D extentBlockCount = util::computeBlockCount(VkExtent3D { extent.width, extent.height, formatInfo->blockSize.depth }, formatInfo->blockSize);
      VkExtent3D alignedExtent = util::computeBlockExtent(extentBlockCount, formatInfo->blockSize);

      alignedExtent = util::snapExtent3D(alignedDstOffset, alignedExtent, dstTexLevelExtent);
      alignedExtent = util::snapExtent3D(alignedSrcOffset, alignedExtent, srcTexLevelExtent);

      VkOffset3D srcOffsetBlockCount = util::computeBlockOffset(alignedSrcOffset, formatInfo->blockSize);
      VkOffset3D dstOffsetBlockCount = util::computeBlockOffset(alignedDstOffset, formatInfo->blockSize);
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

        const Rc<DxvkImage>& srcImage = srcTex->GetImage();
        const Rc<DxvkImage>& dstImage = dstTex->GetImage();

        VkImageSubresourceLayers srcLayers = vk::makeSubresourceLayers(srcTex->GetSubresourceFromIndex(
        formatInfo->aspectMask, src->GetSubresource()));

        VkImageSubresourceLayers dstLayers = vk::makeSubresourceLayers(dstTex->GetSubresourceFromIndex(
              formatInfo->aspectMask, dst->GetSubresource()));

        if (dstImage->info().sampleCount == srcImage->info().sampleCount
          || dstImage->info().sampleCount == 1u) {
          m_device->EmitCs([
            cSrcImage  = srcImage,
            cDstImage  = dstImage,
            cSrcOffset = alignedSrcOffset,
            cDstOffset = alignedDstOffset,
            cSrcLayers = srcLayers,
            cDstLayers = dstLayers,
            cExtent    = alignedExtent,
            cResolve   = dstImage->info().sampleCount == 1u
          ] (DxvkContext* ctx) {
            if (!cResolve) {
              ctx->copyImage(cDstImage, cDstLayers, cDstOffset,
                cSrcImage, cSrcLayers, cSrcOffset, cExtent);
            } else {
              VkImageResolve region;
              region.srcSubresource = cSrcLayers;
              region.srcOffset      = cSrcOffset;
              region.dstSubresource = cDstLayers;
              region.dstOffset      = cDstOffset;
              region.extent         = cExtent;

              ctx->resolveImage(
                cDstImage, cSrcImage, region, cSrcImage->info().format, VK_RESOLVE_MODE_AVERAGE_BIT,
                VK_RESOLVE_MODE_SAMPLE_ZERO_BIT);
            }
          });
        } else {
          DxvkImageViewKey srcViewInfo;
          srcViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
          srcViewInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
          srcViewInfo.format = srcImage->info().format;
          srcViewInfo.aspects = srcLayers.aspectMask;
          srcViewInfo.mipIndex = srcLayers.mipLevel;
          srcViewInfo.mipCount = 1u;
          srcViewInfo.layerIndex = srcLayers.baseArrayLayer;
          srcViewInfo.layerCount = 1u;
          srcViewInfo.packedSwizzle = DxvkImageViewKey::packSwizzle(srcTex->GetMapping().Swizzle);

          DxvkImageViewKey dstViewInfo = srcViewInfo;
          dstViewInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
          dstViewInfo.mipIndex = dstLayers.mipLevel;
          dstViewInfo.layerIndex = dstLayers.baseArrayLayer;

          m_device->EmitCs([
            cSrcView  = srcImage->createView(srcViewInfo),
            cDstView  = dstImage->createView(dstViewInfo),
            cDstOffset = alignedDstOffset,
            cSrcOffset = alignedSrcOffset,
            cExtent    = alignedExtent
          ] (DxvkContext* ctx) {
            std::array<VkOffset3D, 4> offsets = {
              cSrcOffset,
              VkOffset3D { cSrcOffset.x + int32_t(cExtent.width), cSrcOffset.y + int32_t(cExtent.width), cSrcOffset.z + int32_t(cExtent.depth) },
              cDstOffset,
              VkOffset3D { cDstOffset.x + int32_t(cExtent.width), cDstOffset.y + int32_t(cExtent.width), cDstOffset.z + int32_t(cExtent.depth) },
            };
            ctx->blitImageView(
              cDstView, &offsets[2],
              cSrcView, offsets.data(),
              VK_FILTER_NEAREST);
          });
        }
      }
      else if (!srcHasImage && dstHasImage) {
        srcTex->CreateBuffer(true, srcTex->GetTotalSize());
        // Just use the staging buffer upload here.
        m_device->UpdateTextureFromBuffer(dstTex, srcTex, dst->GetSubresource(), src->GetSubresource(), alignedSrcOffset, alignedExtent, alignedDstOffset);
      }
      else if (srcHasImage && !dstHasImage) {
        dstTex->CreateBuffer(true, dstTex->GetTotalSize());
        m_device->EmitCs([
          cDstBuffer = dstTex->GetBuffer(),
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
          ctx->copyImageToBuffer(cDstBuffer, cBufferOffset, cDstPitch, 0,
            VK_FORMAT_UNDEFINED, cSrcImage, cSrcLayers, cSrcOffset, cExtent);
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
