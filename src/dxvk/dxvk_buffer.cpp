#include "dxvk_barrier.h"
#include "dxvk_buffer.h"
#include "dxvk_device.h"

#include <algorithm>

namespace dxvk {
  
  DxvkBuffer::DxvkBuffer(
          DxvkDevice*           device,
    const DxvkBufferCreateInfo& createInfo,
          DxvkMemoryAllocator&  allocator,
          VkMemoryPropertyFlags memFlags)
  : DxvkPagedResource(allocator),
    m_vkd           (device->vkd()),
    m_properties    (memFlags),
    m_shaderStages  (util::shaderStages(createInfo.stages)),
    m_sharingMode   (device->getSharingMode()),
    m_info          (createInfo) {
    m_allocator->registerResource(this);

    // Assign debug name to buffer
    if (device->debugFlags().test(DxvkDebugFlag::Capture)) {
      m_debugName = createDebugName(createInfo.debugName);
      m_info.debugName = m_debugName.c_str();
    } else {
      m_info.debugName = nullptr;
    }

    // Unconditionally enable BDA usage
    m_info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    // Determine whether the image is shareable before creating the resource
    m_shared = m_info.sharing.mode != DxvkSharedHandleMode::None && device->features().khrExternalMemoryWin32;
    m_stableAddress = m_shared;

    // Create and assign actual buffer resource
    assignStorage(allocateStorage());
  }


  DxvkBuffer::DxvkBuffer(
          DxvkDevice*           device,
    const DxvkBufferCreateInfo& createInfo,
    const DxvkBufferImportInfo& importInfo,
          DxvkMemoryAllocator&  allocator,
          VkMemoryPropertyFlags memFlags)
  : DxvkPagedResource(allocator),
    m_vkd           (device->vkd()),
    m_properties    (memFlags),
    m_shaderStages  (util::shaderStages(createInfo.stages)),
    m_sharingMode   (device->getSharingMode()),
    m_info          (createInfo),
    m_stableAddress (true) {
    m_allocator->registerResource(this);

    DxvkAllocationInfo allocationInfo = { };
    allocationInfo.resourceCookie = cookie();

    VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    info.flags = m_info.flags;
    info.usage = m_info.usage;
    info.size = m_info.size;
    m_sharingMode.fill(info);

    assignStorage(allocator.importBufferResource(info, allocationInfo, importInfo));
  }


  DxvkBuffer::~DxvkBuffer() {
    m_allocator->unregisterResource(this);
  }


  bool DxvkBuffer::canRelocate() const {
    return !m_bufferInfo.mapPtr && !m_shared && !m_stableAddress
        && !(m_info.flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT);
  }


  HANDLE DxvkBuffer::sharedHandle() const {
    HANDLE handle = INVALID_HANDLE_VALUE;

    if (!m_shared || m_info.sharing.type == VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT)
      return INVALID_HANDLE_VALUE;

#ifdef _WIN32
    DxvkResourceMemoryInfo memoryInfo = m_storage->getMemoryInfo();

    VkMemoryGetWin32HandleInfoKHR handleInfo = { VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR };
    handleInfo.handleType = m_info.sharing.type;
    handleInfo.memory = memoryInfo.memory;

    if (m_vkd->vkGetMemoryWin32HandleKHR(m_vkd->device(), &handleInfo, &handle) != VK_SUCCESS)
      Logger::warn("DxvkImage::DxvkImage: Failed to get shared handle for image");
#endif

    return handle;
  }


  Rc<DxvkBufferView> DxvkBuffer::createView(
    const DxvkBufferViewKey& info) {
    std::unique_lock lock(m_viewMutex);

    auto entry = m_views.emplace(std::piecewise_construct,
      std::make_tuple(info), std::make_tuple(this, info));

    return &entry.first->second;
  }


  DxvkSparsePageTable* DxvkBuffer::getSparsePageTable() {
    return m_storage->getSparsePageTable();
  }


  Rc<DxvkResourceAllocation> DxvkBuffer::relocateStorage(
          DxvkAllocationModes         mode) {
    // The resource may become non-relocatable even after we allocate new
    // backing storage, but if it already is then don't waste memory.
    if (!canRelocate())
      return nullptr;

    DxvkAllocationInfo allocationInfo = { };
    allocationInfo.resourceCookie = cookie();
    allocationInfo.properties = m_properties;
    allocationInfo.mode = mode;

    VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    info.flags = m_info.flags;
    info.usage = m_info.usage;
    info.size = m_info.size;
    m_sharingMode.fill(info);

    return m_allocator->createBufferResource(info, allocationInfo, nullptr, nullptr);
  }


  void DxvkBuffer::setDebugName(const char* name) {
    if (likely(!m_info.debugName))
      return;

    m_debugName = createDebugName(name);
    m_info.debugName = m_debugName.c_str();

    updateDebugName();
  }


  void DxvkBuffer::updateDebugName() {
    if (m_storage->flags().test(DxvkAllocationFlag::OwnsBuffer)) {
      VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
      nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
      nameInfo.objectHandle = vk::getObjectHandle(m_bufferInfo.buffer);
      nameInfo.pObjectName = m_info.debugName;

      m_vkd->vkSetDebugUtilsObjectNameEXT(m_vkd->device(), &nameInfo);
    }
  }


  std::string DxvkBuffer::createDebugName(const char* name) const {
    return str::format(vk::isValidDebugName(name) ? name : "Buffer", " (", cookie(), ")");
  }


  Rc<DxvkResourceAllocation> DxvkBuffer::allocateStorage(DxvkLocalAllocationCache* cache) {
    DxvkAllocationInfo allocationInfo = { };
    allocationInfo.resourceCookie = cookie();
    allocationInfo.properties = m_properties;

    VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    info.flags = m_info.flags;
    info.usage = m_info.usage;
    info.size = m_info.size;
    m_sharingMode.fill(info);

    // Set up external memory parameters for shared buffers
    VkExternalMemoryImageCreateInfo externalInfo = { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO };

    if (m_shared) {
      externalInfo.pNext = std::exchange(info.pNext, &externalInfo);
      externalInfo.handleTypes = m_info.sharing.type;
    }


    // Set up shared memory properties
    void* sharedMemoryInfo = nullptr;

    VkExportMemoryAllocateInfo sharedExport = { VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO };
    VkImportMemoryWin32HandleInfoKHR sharedImportWin32 = { VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
    VkImportMemoryHostPointerInfoEXT sharedImportHostPointer = { VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT };

    if (m_shared && m_info.sharing.mode == DxvkSharedHandleMode::Export) {
      sharedExport.pNext = std::exchange(sharedMemoryInfo, &sharedExport);
      sharedExport.handleTypes = m_info.sharing.type;
    }

    if (m_shared && m_info.sharing.mode == DxvkSharedHandleMode::Import) {
      if (m_info.sharing.type == VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT) {
        sharedImportHostPointer.pNext = std::exchange(sharedMemoryInfo, &sharedImportHostPointer);
        sharedImportWin32.handleType = m_info.sharing.type;
        sharedImportWin32.handle = m_info.sharing.handle;
      } else {
        sharedImportWin32.pNext = std::exchange(sharedMemoryInfo, &sharedImportWin32);
        sharedImportWin32.handleType = m_info.sharing.type;
        sharedImportWin32.handle = m_info.sharing.handle;
      }
    }

    allocationInfo.handleType = m_info.sharing.type;


    return m_allocator->createBufferResource(info, allocationInfo, cache, sharedMemoryInfo);
  }


  bool DxvkBuffer::canShareBuffer(DxvkDevice* device, const VkBufferCreateInfo& createInfo,
    VkExternalMemoryHandleTypeFlagBits handleType,
    const DxvkSharedHandleInfo& sharingInfo) const {
    if (sharingInfo.mode == DxvkSharedHandleMode::None)
      return false;

    if (handleType == VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT
      || handleType == VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT) {
      if (!device->features().khrExternalMemoryWin32) {
        Logger::err("Failed to create shared resource: VK_KHR_EXTERNAL_MEMORY_WIN32 not supported");
        return false;
      }
    } else if (handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT) {
        if (!device->features().extExternalMemoryHost) {
          Logger::err("Failed to create shared resource: VK_EXT_EXTERNAL_MEMORY_HOST not supported");
          return false;
        }
    } else {
      Logger::err("Failed to create shared resource: Unsupported handle type");
      return false;
    }

    if (createInfo.flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT) {
      Logger::err("Failed to create shared resource: Sharing sparse resources not supported");
      return false;
    }

    return true;
  }




  void DxvkBufferView::updateViews() {
    if (likely(m_key.format))
      m_formatted = m_buffer->m_storage->createBufferView(m_key);

    if (likely(m_buffer->info().usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
      DxvkBufferViewKey rawKey = m_key;
      rawKey.format = VK_FORMAT_UNDEFINED;
      rawKey.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

      m_raw = m_buffer->m_storage->createBufferView(rawKey);
    }

    m_version = m_buffer->m_version;
  }

}
