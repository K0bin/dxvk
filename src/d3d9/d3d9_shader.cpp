#include "d3d9_shader.h"

#include <sm3/sm3_parser.h>
#include <sm3/sm3_converter.h>
#include <sm3/sm3_prepass.h>
#include <sm3/sm3_types.h>

#include "d3d9_caps.h"
#include "d3d9_device.h"
#include "d3d9_util.h"
#include "d3d9_spec_constants.h"

namespace dxvk {

  class D3D9SpecializationConstantLayout : public dxbc_spv::sm3::SpecializationConstantLayout {

  public:

    dxbc_spv::sm3::SpecializationConstantBits getSpecConstantLayout(dxbc_spv::sm3::SpecConstantId id) const {
      D3D9SpecConstantId specConstId = static_cast<D3D9SpecConstantId>(static_cast<uint32_t>(id));
      const auto& layoutEntry = D3D9SpecializationInfo::Layout[specConstId];
      return { layoutEntry.dwordOffset, layoutEntry.bitOffset, layoutEntry.sizeInBits };
    }

    uint32_t getSamplerSpecConstIndex(dxbc_spv::sm3::ShaderType shaderType, uint32_t perShaderSamplerIndex) {
      return shaderType == dxbc_spv::sm3::ShaderType::eVertex
        ? perShaderSamplerIndex + FirstVSSamplerSlot
        : perShaderSamplerIndex;
    }

    uint32_t getOptimizedDwordOffset() const {
      return MaxNumSpecConstants;
    }

  };

  class D3D9ShaderConverter : public DxvkIrShaderConverter {

  public:

    D3D9ShaderConverter(
      const DxvkShaderHash&         ShaderKey,
      const DxvkIrShaderCreateInfo& ModuleInfo,
      const D3D9ShaderOptions&      Options,
      const void*                   pShaderBytecode,
            size_t                  BytecodeLength)
    : m_key(ShaderKey), m_info(ModuleInfo), m_options(Options) {
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
      options.fastFloatEmulation = m_options.fastFloatEmulation;
      options.isSWVP = m_options.swvp;

      dxbc_spv::util::ByteReader reader(m_dxbc.data(), m_dxbc.size());

      D3D9SpecializationConstantLayout specConstLayout;

      dxbc_spv::sm3::Converter converter(reader, specConstLayout, options);

      if (!converter.convertShader(builder))
        throw DxvkError(str::format("Failed to convert shader: ", m_key.toString()));
    }

    uint32_t determineResourceIndex(
            dxbc_spv::ir::ShaderStage stage,
            dxbc_spv::ir::ScalarType  type,
            uint32_t                  regSpace,
            uint32_t                  regIndex) const {

      switch (type) {
        case dxbc_spv::ir::ScalarType::eCbv:
          switch (regIndex) {
            case dxbc_spv::sm3::FastSpecConstCbvRegIdx:
              return D3D9ShaderResourceMapping::getSpecConstantBufferSlot();

            case dxbc_spv::sm3::PSSharedDataCbvRegIdx:
              return D3D9ShaderResourceMapping::computeCbvBinding(stage,
                D3D9ShaderResourceMapping::ConstantBuffers::PSShared);

            /* TODO: Not implemented in the compiler
             case dxbc_spv::sm3::VSClipPlanesCbvRegIdx:
              return D3D9ShaderResourceMapping::computeCbvBinding(stage,
                D3D9ShaderResourceMapping::ConstantBuffers::VSClipPlanes);*/

            case dxbc_spv::sm3::FloatIntCbvRegIdx:
                return D3D9ShaderResourceMapping::computeCbvBinding(stage,
                  D3D9ShaderResourceMapping::ConstantBuffers::VSConstantBuffer);

            case dxbc_spv::sm3::SWVPIntCbvRegIdx:
                return D3D9ShaderResourceMapping::computeCbvBinding(stage,
                D3D9ShaderResourceMapping::ConstantBuffers::VSIntConstantBuffer);

            case dxbc_spv::sm3::SWVPBoolCbvRegIdx:
                return D3D9ShaderResourceMapping::computeCbvBinding(stage,
                  D3D9ShaderResourceMapping::ConstantBuffers::VSBoolConstantBuffer);

            default: break;
          }
          break;

        case dxbc_spv::ir::ScalarType::eSrv:
        case dxbc_spv::ir::ScalarType::eSampler:
          return D3D9ShaderResourceMapping::computeTextureBinding(stage, regIndex);

        default: break;
      }

      Logger::err(str::format("Missing Resource index. Stage: ", stage, ", regSpace: ", regSpace, ", regIndex: ", regIndex));
      return -1u;
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

    D3D9ShaderOptions m_options;

  };

  D3D9CommonShader::D3D9CommonShader() {}

  D3D9CommonShader::D3D9CommonShader(
            D3D9DeviceEx*           pDevice,
      const DxvkShaderHash&         Key,
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

    D3D9ShaderOptions options = { };
    options.swvp = pDevice->CanSWVP();
    options.fastFloatEmulation = pDevice->GetOptions()->d3d9FloatEmulation == D3D9FloatEmulation::Enabled;
    options.vertexFloatConstantBufferAsSSBO = pDevice->VertexFloatConstantBufferAsSSBO();

    DxvkShaderOptions dxvkOptions = pDevice->GetDXVKDevice()->getShaderCompileOptions();
    dxvkOptions.flags.set(DxvkShaderCompileFlag::SemanticIo);

    if (!pDevice->GetOptions()->useFP16)
      dxvkOptions.flags.clr(DxvkShaderCompileFlag::Supports16BitArithmetic);

    DxvkIrShaderCreateInfo createInfo;
    createInfo.options = dxvkOptions;
    // TODO: flat shading mask

    CreateIrShader(pDevice, Key, createInfo, options, pShaderBytecode, BytecodeLength);

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
    if (Prepass.getShaderInfo().getType() == dxbc_spv::sm3::ShaderType::eVertex)
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


  void D3D9CommonShader::CreateIrShader(
          D3D9DeviceEx*           pDevice,
    const DxvkShaderHash&         ShaderKey,
    const DxvkIrShaderCreateInfo& ModuleInfo,
    const D3D9ShaderOptions&      Options,
    const void*                   pShaderBytecode,
          size_t                  BytecodeLength) {
    // Create actual shader converter
    m_shader = pDevice->GetDXVKDevice()->createCachedShader(
      ShaderKey.toString(), ModuleInfo, nullptr);

    if (!m_shader) {
      Rc<D3D9ShaderConverter> converter = new D3D9ShaderConverter(ShaderKey,
        ModuleInfo, Options, pShaderBytecode, BytecodeLength);

      m_shader = pDevice->GetDXVKDevice()->createCachedShader(
        ShaderKey.toString(), ModuleInfo, std::move(converter));
    }
  }


  HRESULT D3D9ShaderModuleSet::GetShaderModule(
          D3D9DeviceEx*           pDevice,
    const DxvkShaderHash&         ShaderKey,
    const dxbc_spv::sm3::Prepass& Prepass,
    const void*                   pShaderBytecode,
          D3D9CommonShader*       pShader) {

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
      pDevice, ShaderKey, Prepass,
      pShaderBytecode, Prepass.getLength());
    
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
