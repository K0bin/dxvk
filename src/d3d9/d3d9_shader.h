#pragma once

#include "../dxso/dxso_module.h"

#include <sm3/sm3_parser.h>
#include <sm3/sm3_prepass.h>
#include <sm3/sm3_types.h>

#include "../dxvk/dxvk_shader.h"
#include "../dxvk/dxvk_shader_ir.h"
#include "../dxvk/dxvk_shader_key.h"

#include "d3d9_resource.h"
#include "d3d9_util.h"
#include "d3d9_mem.h"

#include "../util/util_small_vector.h"

#include <array>

namespace dxvk {


  /**
   * \brief Shader resource mapping
   *
   * Helper class to compute backend resource
   * indices for D3D11 binding slots.
   */
  struct D3D9ShaderResourceMapping {
    enum ConstantBuffers : uint32_t {
      VSConstantBuffer = 0,
      VSFloatConstantBuffer = 0,
      VSIntConstantBuffer = 1,
      VSBoolConstantBuffer = 2,
      VSClipPlanes     = 3,
      VSFixedFunction  = 4,
      VSVertexBlendData = 5,
      VSCount,

      PSConstantBuffer = 0,
      PSFixedFunction  = 1,
      PSShared         = 2,
      PSCount
    };

    template<typename T>
    static uint32_t computeCbvBinding(T stage, uint32_t index) {
      const uint32_t stageOffset = (ConstantBuffers::VSCount + caps::MaxTexturesVS) * computeStageIndex(stage);
      return index + stageOffset;
    }

    template<typename T>
    static uint32_t computeTextureBinding(T stage, uint32_t index) {
      const uint32_t stageIndex = computeStageIndex(stage);
      const uint32_t stageOffset = (ConstantBuffers::VSCount + caps::MaxTexturesVS) * stageIndex;
      return index + stageOffset
        + (stageIndex == 1u
          ? ConstantBuffers::PSCount
          : ConstantBuffers::VSCount);
    }

    static constexpr uint32_t computeStageIndex(dxbc_spv::ir::ShaderStage stage) {
      switch (stage) {
        case dxbc_spv::ir::ShaderStage::eVertex:    return 0u;
        case dxbc_spv::ir::ShaderStage::ePixel:     return 1u;
        default:                                    return -1u;
      }
    }

    static constexpr uint32_t computeStageIndex(D3D9ShaderType stage) {
      return uint32_t(stage);
    }

  };


  /**
   * \brief Common shader object
   * 
   * Stores the compiled SPIR-V shader and the SHA-1
   * hash of the original DXBC shader, which can be
   * used to identify the shader.
   */
  class D3D9CommonShader {

  public:

    D3D9CommonShader();

    D3D9CommonShader(
            D3D9DeviceEx*           pDevice,
            VkShaderStageFlagBits   ShaderStage,
      const DxvkShaderKey&          Key,
      const DxvkIrShaderCreateInfo& ModuleInfo,
      const dxbc_spv::sm3::Prepass& Prepass,
      const void*                   pShaderBytecode,
            size_t                  BytecodeLength);


    Rc<DxvkShader> GetShader() const {
      return m_shader;
    }

    std::string GetName() const {
      return m_shader->debugName();
    }

    const small_vector<dxbc_spv::sm3::Semantic, 4u>& GetInputSingature() const {
      return m_inputSignature;
    }

    const DxsoShaderMetaInfo& GetMeta() const { return m_meta; }
    const DxsoDefinedConstants& GetConstants() const { return m_constants; }

    D3D9ShaderMasks GetShaderMask() const { return D3D9ShaderMasks{ m_usedSamplers, m_usedRTs }; }

    const dxbc_spv::sm3::ShaderInfo& GetInfo() const { return m_info; }

    int32_t GetMaxDefinedFloatConstant() const { return m_maxDefinedFloatConst; }

    int32_t GetMaxDefinedIntConstant() const { return m_maxDefinedIntConst; }

    int32_t GetMaxDefinedBoolConstant() const { return m_maxDefinedBoolConst; }

    VkImageViewType GetImageViewType(uint32_t samplerSlot) const {
      return m_textureTypes[samplerSlot];
    }

  private:

    void CreateIrShader(
            D3D9DeviceEx*          pDevice,
      const DxvkShaderHash&         ShaderKey,
      const DxvkIrShaderCreateInfo& ModuleInfo,
      const void*                   pShaderBytecode,
            size_t                  BytecodeLength);

    small_vector<dxbc_spv::sm3::Semantic, 4u> m_inputSignature;
    uint32_t              m_usedSamplers;
    uint32_t              m_usedRTs;

    std::array<VkImageViewType, 16u> m_textureTypes;

    dxbc_spv::sm3::ShaderInfo       m_info;
    dxbc_spv::sm3::PrepassConstants m_meta;

    dxbc_spv::sm3::ImmediateFloatConstants m_constants;
    int32_t                                m_maxDefinedFloatConst = -1;
    int32_t                                m_maxDefinedIntConst = -1;
    int32_t                                m_maxDefinedBoolConst = -1;

    Rc<DxvkShader> m_shader;

  };

  /**
   * \brief Common shader interface
   * 
   * Implements methods for all D3D11*Shader
   * interfaces and stores the actual shader
   * module object.
   */
  template <typename Base>
  class D3D9Shader : public D3D9DeviceChild<Base> {

  public:

    D3D9Shader(
            D3D9DeviceEx*        pDevice,
            D3D9MemoryAllocator* pAllocator,
      const D3D9CommonShader&    CommonShader,
      const void*                pShaderBytecode,
            uint32_t             BytecodeLength)
      : D3D9DeviceChild<Base>( pDevice )
      , m_shader             ( CommonShader )
      , m_bytecode           ( pAllocator->Alloc(BytecodeLength) )
      , m_bytecodeLength     ( BytecodeLength ) {
      m_bytecode.Map();
      std::memcpy(m_bytecode.Ptr(), pShaderBytecode, BytecodeLength);
      m_bytecode.Unmap();
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) {
      if (ppvObject == nullptr)
        return E_POINTER;

      *ppvObject = nullptr;

      if (riid == __uuidof(IUnknown)
       || riid == __uuidof(Base)) {
        *ppvObject = ref(this);
        return S_OK;
      }

      if (logQueryInterfaceError(__uuidof(Base), riid)) {
        Logger::warn("D3D9Shader::QueryInterface: Unknown interface query");
        Logger::warn(str::format(riid));
      }

      return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetFunction(void* pOut, UINT* pSizeOfData) {
      if (pSizeOfData == nullptr)
        return D3DERR_INVALIDCALL;

      if (pOut == nullptr) {
        *pSizeOfData = m_bytecodeLength;
        return D3D_OK;
      }

      m_bytecode.Map();
      uint32_t copyAmount = std::min(*pSizeOfData, m_bytecodeLength);
      std::memcpy(pOut, m_bytecode.Ptr(), copyAmount);
      m_bytecode.Unmap();

      return D3D_OK;
    }

    const D3D9CommonShader* GetCommonShader() const {
      return &m_shader;
    }

  private:

    D3D9CommonShader m_shader;

    D3D9Memory       m_bytecode;
    uint32_t         m_bytecodeLength;

  };

  // Needs their own classes and not usings for forward decl.

  class D3D9VertexShader final : public D3D9Shader<IDirect3DVertexShader9> {

  public:

    D3D9VertexShader(
            D3D9DeviceEx*        pDevice,
            D3D9MemoryAllocator* pAllocator,
      const D3D9CommonShader&    CommonShader,
      const void*                pShaderBytecode,
            uint32_t             BytecodeLength)
      : D3D9Shader<IDirect3DVertexShader9>( pDevice, pAllocator, CommonShader, pShaderBytecode, BytecodeLength ) { }

  };

  class D3D9PixelShader final : public D3D9Shader<IDirect3DPixelShader9> {

  public:

    D3D9PixelShader(
            D3D9DeviceEx*        pDevice,
            D3D9MemoryAllocator* pAllocator,
      const D3D9CommonShader&    CommonShader,
      const void*                pShaderBytecode,
            uint32_t             BytecodeLength)
      : D3D9Shader<IDirect3DPixelShader9>( pDevice, pAllocator, CommonShader, pShaderBytecode, BytecodeLength ) { }

  };

  /**
   * \brief Shader module set
   * 
   * Some applications may compile the same shader multiple
   * times, so we should cache the resulting shader modules
   * and reuse them rather than creating new ones. This
   * class is thread-safe.
   */
  class D3D9ShaderModuleSet : public RcObject {
    
  public:
    
    HRESULT GetShaderModule(
            D3D9DeviceEx*      pDevice,
      const DxvkShaderHash&    ShaderKey,
      const DxvkShaderOptions& Options,
      const dxbc_spv::sm3::Prepass& Prepass,
      const void*              pShaderBytecode,
            D3D9CommonShader*  pShader);
    
  private:
    
    dxvk::mutex m_mutex;
    
    std::unordered_map<
      DxvkShaderHash,
      D3D9CommonShader,
      DxvkHash, DxvkEq> m_modules;
    
  };

  template<typename T>
  const D3D9CommonShader* GetCommonShader(const T& pShader) {
    return pShader != nullptr ? pShader->GetCommonShader() : nullptr;
  }

}
