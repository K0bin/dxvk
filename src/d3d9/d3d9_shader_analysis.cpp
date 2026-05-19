#include "../dxvk/dxvk_include.h"

#include "d3d9_shader_analysis.h"
#include "d3d9_util.h"

#include <utility>

#include "../util/log/log.h"

namespace dxvk {

  D3D9ShaderAnalysis::D3D9ShaderAnalysis(dxbc_spv::util::ByteReader code, bool isSWVP)
    : m_isSWVP(isSWVP) {
    if (isSWVP)
      m_constants.boolMask = ~0u;

    if (!code) {
      Logger::err("No code chunk found in shader.");
    }

    if (Parser parser = Parser(code)) {
      m_length = 0u;
      RunAnalysis(parser);
    } else {
      Logger::err("Failed to parse code chunk.");
    }
  }

  D3D9ShaderAnalysis::D3D9ShaderAnalysis(D3D9ShaderAnalysis&& other)
    : m_isSWVP(other.m_isSWVP), m_length(other.m_length), m_shaderInfo(other.m_shaderInfo),
      m_constants(other.m_constants), m_immediateConstants(std::move(other.m_immediateConstants)),
      m_usedRTs(other.m_usedRTs), m_usedSamplers(other.m_usedSamplers), m_imageViewTypes(other.m_imageViewTypes),
      m_inputSignature(std::move(other.m_inputSignature)) { }

  D3D9ShaderAnalysis::D3D9ShaderAnalysis(const D3D9ShaderAnalysis& other)
    : m_isSWVP(other.m_isSWVP), m_length(other.m_length), m_shaderInfo(other.m_shaderInfo),
      m_constants(other.m_constants), m_immediateConstants(other.m_immediateConstants), m_usedRTs(other.m_usedRTs),
      m_usedSamplers(other.m_usedSamplers), m_imageViewTypes(other.m_imageViewTypes),
      m_inputSignature(other.m_inputSignature) { }

  bool D3D9ShaderAnalysis::RunAnalysis(Parser& parser) {
    m_shaderInfo = parser.getShaderInfo();

    while (parser) {
      Instruction instruction = parser.parseInstruction();

      if (!HandleInstruction(instruction))
        return false;
    }

    m_length = parser.getByteOffset();

    // Shift up these sampler bits so we can just
    // do an or per-draw in the device.
    // We shift by 17 because 16 ps samplers + 1 dmap (tess)
    if (m_shaderInfo.getType() == ShaderType::eVertex)
      m_usedSamplers <<= FirstVSSamplerSlot;

    return true;
  }

  bool D3D9ShaderAnalysis::HandleInstruction(const Instruction& op) {
    /* Determine whether we're accessing float constants dynamically
     * because in that case, we'll need to copy the immediate constants
     * into the constant buffer inside DXVK. */
    for (uint32_t i = 0u; i < op.getSrcCount(); i++) {
      const auto& src = op.getSrc(i);
      auto registerType = src.getRegisterType();
      uint32_t index = src.getIndex();

      if (GetShaderInfo().getType() == ShaderType::ePixel
        && op.hasDst()
        && op.getDst().getRegisterType() == RegisterType::eColorOut) {
        m_usedRTs |= 1u << op.getDst().getIndex();
      }

      /* The indices start counting at 1 to differentiate between a shader
       * that doesn't access any constants at all and one that accesses the
       * first one. */

      if (registerType == RegisterType::eConstInt) {
        m_constants.maxIntIndex = std::max(m_constants.maxIntIndex, int32_t(index));
        continue;
      }

      if (registerType == RegisterType::eConstBool) {
        m_constants.maxBoolIndex = std::max(m_constants.maxBoolIndex, int32_t(index));
        if (!m_isSWVP)
          m_constants.boolMask |= 1u << index;

        continue;
      }

      if (registerType != RegisterType::eConst
        && registerType != RegisterType::eConst2
        && registerType != RegisterType::eConst3
        && registerType != RegisterType::eConst4)
        continue;

      m_constants.maxFloatIndex = std::max(m_constants.maxFloatIndex, int32_t(index));

      if (!src.hasRelativeAddressing())
        continue;

      uint32_t hwvpFloatConstantsCount = GetShaderInfo().getType() == ShaderType::ePixel ? MaxFloatConstantsPS : MaxFloatConstantsVS;

      m_constants.maxFloatIndex = std::max(
        m_constants.maxFloatIndex,
        int32_t(m_isSWVP ? MaxFloatConstantsSoftware : hwvpFloatConstantsCount)
      );
      m_constants.floatsAccessedDynamically = true;
    }

    switch (op.getOpCode()) {
      case OpCode::eDef:
      case OpCode::eDefI:
      case OpCode::eDefB:
        if (!HandleDef(op))
          return false;
        break;

      case OpCode::eTexLd:
      case OpCode::eTexBem:
      case OpCode::eTexBemL:
      case OpCode::eTexReg2Ar:
      case OpCode::eTexReg2Gb:
      case OpCode::eTexM3x2Tex:
      case OpCode::eTexM3x3Tex:
      case OpCode::eTexM3x3Spec:
      case OpCode::eTexM3x3VSpec:
      case OpCode::eTexReg2Rgb:
      case OpCode::eTexDp3Tex:
      case OpCode::eTexM3x2Depth:
      case OpCode::eTexDp3:
      case OpCode::eTexM3x3:
      case OpCode::eTexLdd:
      case OpCode::eTexLdl:
        if (!HandleTextureSample(op))
          return false;
        break;

      case OpCode::eDcl:
        if (!HandleDcl(op))
          return false;
        break;

      default: break;
    }

    return true;
  }

  bool D3D9ShaderAnalysis::HandleDef(const Instruction& op) {
    dxbc_spv_assert(op.hasDst());
    uint32_t index = op.getDst().getIndex();

    if (op.getOpCode() == OpCode::eDef) {
      m_immediateConstants.maxFloatIndex = std::max(m_immediateConstants.maxFloatIndex, int32_t(index));

      dxbc_spv_assert(op.hasImm());
      auto imm = op.getImm();

      Vector4 value = {
        imm.getImmediate<float>(0u), imm.getImmediate<float>(1u),
        imm.getImmediate<float>(2u), imm.getImmediate<float>(3u)
      };
      m_immediateConstants.floats.push_back({ index, value });
    } else if (op.getOpCode() == OpCode::eDefI) {
      m_immediateConstants.maxIntIndex = std::max(m_immediateConstants.maxIntIndex, int32_t(index));
    } else if (op.getOpCode() == OpCode::eDefB) {
      m_immediateConstants.maxBoolIndex = std::max(m_immediateConstants.maxBoolIndex, int32_t(index));
    } else {
      return false;
    }

    return true;
  }


  bool D3D9ShaderAnalysis::HandleTextureSample(const Instruction& op) {
    uint32_t samplerIndex;
    auto dst = op.getDst();
    Operand src1;
    if (op.getSrcCount() >= 2u)
      src1 = op.getSrc(1u);

    switch (op.getOpCode()) {
      case OpCode::eTexLd:
        if (GetShaderInfo().getVersion().first <= 1u)
          samplerIndex = dst.getIndex();
        else
          samplerIndex = src1.getIndex();
        break;

      case OpCode::eTexLdl:
      case OpCode::eTexLdd:
        samplerIndex = src1.getIndex();
        break;

      case OpCode::eTexReg2Ar:
      case OpCode::eTexReg2Gb:
      case OpCode::eTexReg2Rgb:
      case OpCode::eTexM3x2Tex:
      case OpCode::eTexM3x3Tex:
      case OpCode::eTexM3x3:
      case OpCode::eTexM3x2Depth:
      case OpCode::eTexM3x3Spec:
      case OpCode::eTexM3x3VSpec:
      case OpCode::eTexDp3Tex:
      case OpCode::eTexDp3:
      case OpCode::eTexBem:
      case OpCode::eTexBemL:
        samplerIndex = dst.getIndex();
        break;

      default: return false;
    }

    m_usedSamplers |= 1u << samplerIndex;

    return true;
  }


  bool D3D9ShaderAnalysis::HandleDcl(const Instruction& op) {
    dxbc_spv_assert(op.hasDcl());
    const auto& dcl = op.getDcl();
    dxbc_spv_assert(op.hasDst());
    const auto& dst = op.getDst();

    RegisterType registerType = dst.getRegisterType();
    uint32_t index = dst.getIndex();

    if (registerType == RegisterType::eSampler) {
      switch (dcl.getTextureType()) {
        case TextureType::eTexture3D:
          m_imageViewTypes[index] = VK_IMAGE_VIEW_TYPE_3D;
          break;
        case TextureType::eTextureCube:
          m_imageViewTypes[index] = VK_IMAGE_VIEW_TYPE_CUBE;
          break;
        case TextureType::eTexture2D:
        default:
          m_imageViewTypes[index] = VK_IMAGE_VIEW_TYPE_2D;
          break;
      }
      m_usedSamplers |= 1u << index;
      return true;
    }

    if (registerType == RegisterType::eInput && GetShaderInfo().getType() == ShaderType::eVertex) {
      m_inputSignature.push_back({ uint32_t(m_inputSignature.size()) + 12u, { dcl.getSemanticUsage(), dcl.getSemanticIndex() } });
      return true;
    }

    return true;
  }


  D3D9ShaderAnalysis& D3D9ShaderAnalysis::operator=(const D3D9ShaderAnalysis& other) {
    if (this == &other)
      return *this;

    m_isSWVP = other.m_isSWVP;
    m_length = other.m_length;
    m_shaderInfo = other.m_shaderInfo;
    m_constants = other.m_constants;
    m_immediateConstants = other.m_immediateConstants;
    m_usedRTs = other.m_usedRTs;
    m_usedSamplers = other.m_usedSamplers;
    m_imageViewTypes = other.m_imageViewTypes;
    m_inputSignature = other.m_inputSignature;

    return *this;
  }

  D3D9ShaderAnalysis& D3D9ShaderAnalysis::operator=(D3D9ShaderAnalysis&& other) {
    if (this == &other)
      return *this;

    m_isSWVP = other.m_isSWVP;
    m_length = other.m_length;
    m_shaderInfo = other.m_shaderInfo;
    m_constants = other.m_constants;
    m_immediateConstants = std::move(other.m_immediateConstants);
    m_usedRTs = other.m_usedRTs;
    m_usedSamplers = other.m_usedSamplers;
    m_imageViewTypes = other.m_imageViewTypes;
    m_inputSignature = std::move(other.m_inputSignature);

    return *this;
  }

}
