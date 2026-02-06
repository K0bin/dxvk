#include "d3d9_shader.h"

#include <sm3/sm3_interface.h>
#include <sm3/sm3_parser.h>
#include <sm3/sm3_prepass.h>

#include "d3d9_caps.h"
#include "d3d9_device.h"
#include "d3d9_util.h"

namespace dxvk {

  class D3D9ShaderConverter : public DxvkIrShaderConverter {

  public:

    D3D9ShaderConverter(
      const DxvkShaderHash&         ShaderKey,
      const DxvkIrShaderCreateInfo& ModuleInfo,
      const void*                   pShaderBytecode,
            size_t                  BytecodeLength,
            bool                    FastFloatEmulation,
            bool                    SWVP)
    : m_key(ShaderKey), m_info(ModuleInfo), m_fastFloatEmulation(FastFloatEmulation), m_SWVP(SWVP) {
      m_dxbc.resize(BytecodeLength);
      std::memcpy(m_dxbc.data(), pShaderBytecode, BytecodeLength);
    }

    ~D3D9ShaderConverter() { }

    void convertShader(
            dxbc_spv::ir::Builder&    builder) {
      auto debugName = m_key.toString();

      dxbc_spv::sm3::Converter::Options options = { };
      options.name = debugName.c_str();
      options.includeDebugNames = true;
      options.fastFloatEmulation = m_fastFloatEmulation;
      options.isSWVP = m_SWVP;

      dxbc_spv::util::ByteReader reader(m_dxbc.data(), m_dxbc.size());

      dxbc_spv::sm3::Converter converter(reader, TODO, options);

      if (!converter.convertShader(builder))
        throw DxvkError(str::format("Failed to convert shader: ", m_key.toString()));
    }

    uint32_t determineResourceIndex(
            dxbc_spv::ir::ShaderStage stage,
            dxbc_spv::ir::ScalarType  type,
            uint32_t                  regSpace,
            uint32_t                  regIndex) const {

      switch (regSpace) {
        case dxbc_spv::sm3::TextureBindingsRegSpace:




          break;

      }

      if (regSpace == dxbc_spv::sm3::ConstantBufferRegSpace) {

      }

      switch (type) {
        case dxbc_spv::ir::ScalarType::eSampler:
          return D3D11ShaderResourceMapping::computeSamplerBinding(stage, regIndex);
        case dxbc_spv::ir::ScalarType::eCbv:
          return D3D11ShaderResourceMapping::computeCbvBinding(stage, regIndex);
        case dxbc_spv::ir::ScalarType::eSrv:
          return D3D11ShaderResourceMapping::computeSrvBinding(stage, regIndex);
        case dxbc_spv::ir::ScalarType::eUav:
          return D3D11ShaderResourceMapping::computeUavBinding(stage, regIndex);
        case dxbc_spv::ir::ScalarType::eUavCounter:
          return D3D11ShaderResourceMapping::computeUavCounterBinding(stage, regIndex);
        default:
          return -1u;
      }
    }

    void dumpSource(const std::string& path) const {
      std::ofstream file(str::topath(str::format(path, "/", m_key.toString(), ".sm3_dxbc").c_str()).c_str(), std::ios_base::trunc | std::ios_base::binary);
      file.write(reinterpret_cast<const char*>(m_dxbc.data()), m_dxbc.size());
    }

    std::string getDebugName() const {
      return m_key.toString();
    }

  private:

    std::vector<uint8_t> m_dxbc;

    DxvkShaderHash          m_key;
    DxvkIrShaderCreateInfo  m_info;

    bool                    m_fastFloatEmulation = false;
    bool                    m_SWVP               = false;

  };

  D3D9CommonShader::D3D9CommonShader() {}

  D3D9CommonShader::D3D9CommonShader(
            D3D9DeviceEx*           pDevice,
            VkShaderStageFlagBits   ShaderStage,
      const DxvkShaderKey&          Key,
      const DxvkIrShaderCreateInfo& ModuleInfo,
      const dxbc_spv::sm3::Prepass& Prepass,
      const void*                   pShaderBytecode,
            size_t                  BytecodeLength) {

    const std::string name = Key.toString();
    Logger::debug(str::format("Compiling shader ", name));
    
    // If requested by the user, dump both the raw DXBC
    // shader and the compiled SPIR-V module to a file.
    const std::string& dumpPath = pDevice->GetOptions()->shaderDumpPath;
    
    if (dumpPath.size() != 0) {
      DxsoReader reader(
        reinterpret_cast<const char*>(pShaderBytecode));

      reader.store(std::ofstream(str::topath(str::format(dumpPath, "/", name, ".dxso").c_str()).c_str(),
        std::ios_base::binary | std::ios_base::trunc), BytecodeLength);

      char comment[2048];
      Com<ID3DBlob> blob;
      HRESULT hr = DisassembleShader(
        pShaderBytecode,
        TRUE,
        comment, 
        &blob);
      
      if (SUCCEEDED(hr)) {
        std::ofstream disassembledOut(str::topath(str::format(dumpPath, "/", name, ".dxso.dis").c_str()).c_str(), std::ios_base::binary | std::ios_base::trunc);
        disassembledOut.write(
          reinterpret_cast<const char*>(blob->GetBufferPointer()),
          blob->GetBufferSize());
      }
    }
    
    // Decide whether we need to create a pass-through
    // geometry shader for vertex shader stream output

    const D3D9ConstantLayout& constantLayout = ShaderStage == VK_SHADER_STAGE_VERTEX_BIT
      ? pDevice->GetVertexConstantLayout()
      : pDevice->GetPixelConstantLayout();
    m_shader       = pModule->compile(*pDxsoModuleInfo, name, AnalysisInfo, constantLayout);
    m_usedSamplers = Prepass.getSamplerMask();

    m_inputSignature.reserve(Prepass.getInputSignatureSize());
    for (uint32_t i = 0; i < Prepass.getInputSignatureSize(); i++) {
      m_inputSignature.push_back(Prepass.getInputSignatureElement(i));
    }

    for (uint32_t i = 0u; i < m_textureTypes.size(); i++) {
      auto textureType = Prepass.getTextureType(i);
      switch (textureType) {
        case dxbc_spv::sm3::TextureType::eTexture2D:
          m_textureTypes[i] = VK_IMAGE_VIEW_TYPE_2D;
          break;
        case dxbc_spv::sm3::TextureType::eTexture3D:
          m_textureTypes[i] = VK_IMAGE_VIEW_TYPE_3D;
          break;
        case dxbc_spv::sm3::TextureType::eTextureCube:
          m_textureTypes[i] = VK_IMAGE_VIEW_TYPE_CUBE;
          break;
      }
    }

    // Shift up these sampler bits so we can just
    // do an or per-draw in the device.
    // We shift by 17 because 16 ps samplers + 1 dmap (tess)
    if (ShaderStage == VK_SHADER_STAGE_VERTEX_BIT)
      m_usedSamplers <<= FirstVSSamplerSlot;

    m_usedRTs              = Prepass.getRenderTargetMask();

    m_info                 = Prepass.getShaderInfo();
    m_meta                 = Prepass.getConstantsInfo();
    m_constants            = Prepass.getImmediateConstants().floats;
    m_maxDefinedFloatConst = Prepass.getImmediateConstants().maxFloatIndex;
    m_maxDefinedIntConst   = Prepass.getImmediateConstants().maxIntIndex;
    m_maxDefinedBoolConst  = Prepass.getImmediateConstants().maxBoolIndex;

    if (dumpPath.size() != 0) {
      std::ofstream dumpStream(
        str::topath(str::format(dumpPath, "/", name, ".spv").c_str()).c_str(),
        std::ios_base::binary | std::ios_base::trunc);
      
      m_shader->dump(dumpStream);
    }

    pDevice->GetDXVKDevice()->registerShader(m_shader);
  }


  HRESULT D3D9ShaderModuleSet::GetShaderModule(
          D3D9DeviceEx*      pDevice,
    const DxvkShaderHash&    ShaderKey,
    const DxvkShaderOptions& Options,
    const dxbc_spv::sm3::Prepass& Prepass,
    const void*              pShaderBytecode,
          D3D9CommonShader*  pShader) {

    // Use the shader's unique key for the lookup
    { std::unique_lock<dxvk::mutex> lock(m_mutex);
      
      auto entry = m_modules.find(ShaderKey);
      if (entry != m_modules.end()) {
        *pShader = entry->second;
        return D3D_OK;
      }
    }
    
    // This shader has not been compiled yet, so we have to create a
    // new module. This takes a while, so we won't lock the structure.
    *pShader = D3D9CommonShader(
      pDevice, ShaderStage, ShaderKey,
      pDxbcModuleInfo, pShaderBytecode,
      info, &module);
    
    // Insert the new module into the lookup table. If another thread
    // has compiled the same shader in the meantime, we should return
    // that object instead and discard the newly created module.
    std::unique_lock<dxvk::mutex> lock(m_mutex);

    auto status = m_modules.insert({ ShaderKey, *pShader });
    if (!status.second) {
      *pShader = status.first->second;
    }

    return D3D_OK;
  }

}
