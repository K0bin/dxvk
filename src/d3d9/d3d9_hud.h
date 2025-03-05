#pragma once

#include "d3d9_device.h"
#include "../dxvk/hud/dxvk_hud_item.h"

namespace dxvk::hud {

  /**
   * \brief HUD item to display sampler count
   */
  class HudSamplerCount : public HudItem {

  public:

    HudSamplerCount(D3D9DeviceEx* device);

    void update(dxvk::high_resolution_clock::time_point time);

    HudPos render(
      const DxvkContextObjects& ctx,
      const HudPipelineKey&     key,
      const HudOptions&         options,
            HudRenderer&        renderer,
            HudPos              position);

  private:

    D3D9DeviceEx* m_device;

    std::string m_samplerCount;

  };

  /**
   * \brief HUD item to display unmappable memory
   */
  class HudTextureMemory : public HudItem {
    constexpr static int64_t UpdateInterval = 500'000;
  public:

    HudTextureMemory(D3D9DeviceEx* device);

    void update(dxvk::high_resolution_clock::time_point time);

    HudPos render(
      const DxvkContextObjects& ctx,
      const HudPipelineKey&     key,
      const HudOptions&         options,
            HudRenderer&        renderer,
            HudPos              position);

  private:

    D3D9DeviceEx* m_device;

    uint32_t m_maxAllocated = 0;
    uint32_t m_maxUsed      = 0;
    uint32_t m_maxMapped    = 0;

    dxvk::high_resolution_clock::time_point m_lastUpdate
      = dxvk::high_resolution_clock::now();

    std::string m_allocatedString;
    std::string m_mappedString;

  };


  /**
   * \brief HUD item to display amount of generated fixed function shaders
   */
  class HudFixedFunctionShaders : public HudItem {

  public:

    HudFixedFunctionShaders(D3D9DeviceEx* device);

    void update(dxvk::high_resolution_clock::time_point time);

    HudPos render(
      const DxvkContextObjects& ctx,
      const HudPipelineKey&     key,
      const HudOptions&         options,
            HudRenderer&        renderer,
            HudPos              position);

  private:

    D3D9DeviceEx* m_device;

    std::string m_ffShaderCount;

  };


  /**
   * \brief HUD item to whether or not we're in SWVP mode
   */
  class HudSWVPState : public HudItem {

  public:

    HudSWVPState(D3D9DeviceEx* device);

    void update(dxvk::high_resolution_clock::time_point time);

    HudPos render(
      const DxvkContextObjects& ctx,
      const HudPipelineKey&     key,
      const HudOptions&         options,
            HudRenderer&        renderer,
            HudPos              position);

  private:

    D3D9DeviceEx* m_device;

    std::string m_isSWVPText;

  };


  /**
   * \brief HUD item to show some debug info for constants
   */
  class HudConsts : public HudItem {
    constexpr static int64_t UpdateInterval = 5'000'000;
  public:

  HudConsts(D3D9DeviceEx* device);

    void update(dxvk::high_resolution_clock::time_point time);

    HudPos render(
      const DxvkContextObjects& ctx,
      const HudPipelineKey&     key,
      const HudOptions&         options,
            HudRenderer&        renderer,
            HudPos              position);

  private:

    D3D9DeviceEx* m_device;

    dxvk::high_resolution_clock::time_point m_lastUpdate
      = dxvk::high_resolution_clock::now();

    uint32_t                        m_maxVSFloatConstsChangedPerFrame = 0;
    uint32_t                        m_maxVSIntConstsChangedPerFrame = 0;
    uint32_t                        m_maxVSBoolConstsChangedPerFrame = 0;
    uint32_t                        m_maxPSFloatConstsChangedPerFrame = 0;
    uint32_t                        m_maxPSIntConstsChangedPerFrame = 0;
    uint32_t                        m_maxPSBoolConstsChangedPerFrame = 0;
    uint32_t                        m_maxDrawCallsPerFrame = 0;

    uint32_t                        m_prevVSFloatConstsChanged = 0;
    uint32_t                        m_prevVSIntConstsChanged = 0;
    uint32_t                        m_prevVSBoolConstsChanged = 0;
    uint32_t                        m_prevPSFloatConstsChanged = 0;
    uint32_t                        m_prevPSIntConstsChanged = 0;
    uint32_t                        m_prevPSBoolConstsChanged = 0;
    uint32_t                        m_prevDrawCalls = 0;

    std::string                     m_vsFloatConstString;
    std::string                     m_vsIntConstString;
    std::string                     m_vsBoolConstString;
    std::string                     m_psFloatConstString;
    std::string                     m_psIntConstString;
    std::string                     m_psBoolConstString;

  };

}