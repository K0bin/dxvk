#include "d3d9_hud.h"

namespace dxvk::hud {

  HudSamplerCount::HudSamplerCount(D3D9DeviceEx* device)
    : m_device       (device)
    , m_samplerCount ("0"){

  }


  void HudSamplerCount::update(dxvk::high_resolution_clock::time_point time) {
    DxvkSamplerStats stats = m_device->GetDXVKDevice()->getSamplerStats();
    m_samplerCount = str::format(stats.totalCount);
  }


  HudPos HudSamplerCount::render(
    const DxvkContextObjects& ctx,
    const HudPipelineKey&     key,
    const HudOptions&         options,
          HudRenderer&        renderer,
          HudPos              position) {
    position.y += 16;
    renderer.drawText(16, position, 0xffc0ff00u, "Samplers:");
    renderer.drawText(16, { position.x + 120, position.y }, 0xffffffffu, m_samplerCount);

    position.y += 8;
    return position;
  }

  HudTextureMemory::HudTextureMemory(D3D9DeviceEx* device)
  : m_device          (device)
  , m_allocatedString ("")
  , m_mappedString    ("") { }


  void HudTextureMemory::update(dxvk::high_resolution_clock::time_point time) {
    D3D9MemoryAllocator* allocator = m_device->GetAllocator();

    m_maxAllocated = std::max(m_maxAllocated, allocator->AllocatedMemory());
    m_maxUsed = std::max(m_maxUsed, allocator->UsedMemory());
    m_maxMapped = std::max(m_maxMapped, allocator->MappedMemory());

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(time - m_lastUpdate);

    if (elapsed.count() < UpdateInterval)
      return;

    m_allocatedString = str::format(m_maxAllocated >> 20, " MB (Used: ", m_maxUsed >> 20, " MB)");
    m_mappedString = str::format(m_maxMapped >> 20, " MB");
    m_maxAllocated = 0;
    m_maxUsed = 0;
    m_maxMapped = 0;
    m_lastUpdate = time;
  }


  HudPos HudTextureMemory::render(
    const DxvkContextObjects& ctx,
    const HudPipelineKey&     key,
    const HudOptions&         options,
          HudRenderer&        renderer,
          HudPos              position) {
    position.y += 16;
    renderer.drawText(16, position, 0xffc0ff00u, "Mappable:");
    renderer.drawText(16, { position.x + 120, position.y }, 0xffffffffu, m_allocatedString);

    position.y += 20;
    renderer.drawText(16, position, 0xffc0ff00u, "Mapped:");
    renderer.drawText(16, { position.x + 120, position.y }, 0xffffffffu, m_mappedString);

    position.y += 8;
    return position;
  }

  HudFixedFunctionShaders::HudFixedFunctionShaders(D3D9DeviceEx* device)
  : m_device        (device)
  , m_ffShaderCount ("") {}


  void HudFixedFunctionShaders::update(dxvk::high_resolution_clock::time_point time) {
    m_ffShaderCount = str::format(
      "VS: ", m_device->GetFixedFunctionVSCount(),
      " FS: ", m_device->GetFixedFunctionFSCount(),
      " SWVP: ", m_device->GetSWVPShaderCount()
    );
  }


  HudPos HudFixedFunctionShaders::render(
    const DxvkContextObjects& ctx,
    const HudPipelineKey&     key,
    const HudOptions&         options,
          HudRenderer&        renderer,
          HudPos              position) {
    position.y += 16;
    renderer.drawText(16, position, 0xffc0ff00u, "FF Shaders:");
    renderer.drawText(16, { position.x + 155, position.y }, 0xffffffffu, m_ffShaderCount);

    position.y += 8;
    return position;
  }


  HudSWVPState::HudSWVPState(D3D9DeviceEx* device)
          : m_device          (device)
          , m_isSWVPText ("") {}



  void HudSWVPState::update(dxvk::high_resolution_clock::time_point time) {
    if (m_device->IsSWVP()) {
      if (m_device->CanOnlySWVP()) {
        m_isSWVPText = "SWVP";
      } else {
        m_isSWVPText = "SWVP (Mixed)";
      }
    } else {
      if (m_device->CanSWVP()) {
        m_isSWVPText = "HWVP (Mixed)";
      } else {
        m_isSWVPText = "HWVP";
      }
    }
  }


  HudPos HudSWVPState::render(
    const DxvkContextObjects& ctx,
    const HudPipelineKey&     key,
    const HudOptions&         options,
          HudRenderer&        renderer,
          HudPos              position) {
    position.y += 16;
    renderer.drawText(16, position, 0xffc0ff00u, "Vertex Processing:");
    renderer.drawText(16, { position.x + 240, position.y }, 0xffffffffu, m_isSWVPText);

    position.y += 8;
    return position;
  }


  HudConsts::HudConsts(D3D9DeviceEx* device)
          : m_device          (device)
          , m_vsFloatConstString ("")
          , m_vsIntConstString ("")
          , m_vsBoolConstString ("")
          , m_psFloatConstString ("")
          , m_psIntConstString ("")
          , m_psBoolConstString ("") {}



  void HudConsts::update(dxvk::high_resolution_clock::time_point time) {
    // If I actually want to merge this, fix the thread safety...

    uint64_t vsFloatConstsChanged = m_device->vsFloatConstsChanged();
    uint64_t vsIntConstsChanged = m_device->vsIntConstsChanged();
    uint64_t vsBoolConstsChanged = m_device->vsBoolConstsChanged();
    uint64_t psFloatConstsChanged = m_device->psFloatConstsChanged();
    uint64_t psIntConstsChanged = m_device->psIntConstsChanged();
    uint64_t psBoolConstsChanged = m_device->psBoolConstsChanged();

    m_maxVSFloatConstsChangedPerFrame = std::max(m_maxVSFloatConstsChangedPerFrame, uint32_t(vsFloatConstsChanged - m_prevVSFloatConstsChanged));
    m_maxVSIntConstsChangedPerFrame = std::max(m_maxVSIntConstsChangedPerFrame, uint32_t(vsIntConstsChanged - m_prevVSIntConstsChanged));
    m_maxVSBoolConstsChangedPerFrame = std::max(m_maxVSBoolConstsChangedPerFrame, uint32_t(vsBoolConstsChanged - m_prevVSBoolConstsChanged));
    m_maxPSFloatConstsChangedPerFrame = std::max(m_maxPSFloatConstsChangedPerFrame, uint32_t(psFloatConstsChanged - m_prevPSFloatConstsChanged));
    m_maxPSIntConstsChangedPerFrame = std::max(m_maxPSIntConstsChangedPerFrame, uint32_t(psIntConstsChanged - m_prevPSIntConstsChanged));
    m_maxPSBoolConstsChangedPerFrame = std::max(m_maxPSBoolConstsChangedPerFrame, uint32_t(psBoolConstsChanged - m_prevPSBoolConstsChanged));

    m_prevVSFloatConstsChanged = vsFloatConstsChanged;
    m_prevVSIntConstsChanged = vsIntConstsChanged;
    m_prevVSBoolConstsChanged = vsBoolConstsChanged;
    m_prevPSFloatConstsChanged = psFloatConstsChanged;
    m_prevPSIntConstsChanged = psIntConstsChanged;
    m_prevPSBoolConstsChanged = psBoolConstsChanged;

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(time - m_lastUpdate);

    if (elapsed.count() < UpdateInterval && m_vsFloatConstString.size() != 0)
      return;

    m_vsFloatConstString = str::format(m_maxVSFloatConstsChangedPerFrame, " (Copying: ", m_device->maxVSFloatConst(), " per draw)");
    m_vsIntConstString = str::format(m_maxVSIntConstsChangedPerFrame, " (Copying: ", m_device->maxVSIntConst(), " per draw)");
    m_vsBoolConstString = str::format(m_maxVSBoolConstsChangedPerFrame, " (Copying: ", m_device->maxVSBoolConst(), " per draw)");
    m_psFloatConstString = str::format(m_maxPSFloatConstsChangedPerFrame, " (Copying: ", m_device->maxPSFloatConst(), " per draw)");
    m_psIntConstString = str::format(m_maxPSIntConstsChangedPerFrame, " (Copying: ", m_device->maxPSIntConst(), " per draw)");
    m_psBoolConstString = str::format(m_maxPSBoolConstsChangedPerFrame, " (Copying: ", m_device->maxPSBoolConst(), " per draw)");

    m_maxVSFloatConstsChangedPerFrame = 0;
    m_maxVSIntConstsChangedPerFrame = 0;
    m_maxVSBoolConstsChangedPerFrame = 0;
    m_maxPSFloatConstsChangedPerFrame = 0;
    m_maxPSIntConstsChangedPerFrame = 0;
    m_maxPSBoolConstsChangedPerFrame = 0;
  }


  HudPos HudConsts::render(
    const DxvkContextObjects& ctx,
    const HudPipelineKey&     key,
    const HudOptions&         options,
          HudRenderer&        renderer,
          HudPos              position) {
    position.y += 16;
    renderer.drawText(16, position, 0xffc0ff00u, "VS Float Consts:");
    renderer.drawText(16, { position.x + 240, position.y }, 0xffffffffu, m_vsFloatConstString);
    position.y += 20;
    renderer.drawText(16, position, 0xffc0ff00u, "VS Int Consts:");
    renderer.drawText(16, { position.x + 240, position.y }, 0xffffffffu, m_vsIntConstString);
    position.y += 20;
    renderer.drawText(16, position, 0xffc0ff00u, "VS Bool Consts:");
    renderer.drawText(16, { position.x + 240, position.y }, 0xffffffffu, m_vsBoolConstString);
    position.y += 20;
    renderer.drawText(16, position, 0xffc0ff00u, "PS Float Consts:");
    renderer.drawText(16, { position.x + 240, position.y }, 0xffffffffu, m_psFloatConstString);
    position.y += 20;
    renderer.drawText(16, position, 0xffc0ff00u, "PS Int Consts:");
    renderer.drawText(16, { position.x + 240, position.y }, 0xffffffffu, m_psIntConstString);
    position.y += 20;
    renderer.drawText(16, position, 0xffc0ff00u, "PS Bool Consts:");
    renderer.drawText(16, { position.x + 240, position.y }, 0xffffffffu, m_psBoolConstString);

    position.y += 8;
    return position;
  }

}
