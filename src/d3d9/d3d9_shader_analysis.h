#pragma once

#include <util/util_byte_stream.h>
#include <util/util_small_vector.h>

#include <sm3/sm3_types.h>
#include <sm3/sm3_parser.h>
#include <sm3/sm3_resources.h>

#include <vector>

using namespace dxbc_spv::sm3;
using dxbc_spv::util::small_vector;

namespace dxvk {

struct ImmediateFloatConstant {
  uint32_t index;

  std::array<float, 4u> value;
};

using ImmediateFloatConstants = std::vector<ImmediateFloatConstant>;

struct ImmediateConstants {
  int32_t maxFloatIndex = -1;
  int32_t maxIntIndex   = -1;
  int32_t maxBoolIndex  = -1;

  ImmediateFloatConstants floats;
};

struct ShaderConstantsInfo {
  bool floatsAccessedDynamically = false;

  int32_t maxFloatIndex = -1;
  int32_t maxIntIndex   = -1;
  int32_t maxBoolIndex  = -1;

  uint32_t boolMask = 0u;
};

struct InputSignatureElement {
  uint32_t location;
  Semantic semantic;
};

using RenderTargetMask = uint8_t;
using SamplerMask      = uint32_t;
using Signature        = small_vector<InputSignatureElement, 16u>;

class D3D9ShaderAnalysis {

public:

  D3D9ShaderAnalysis(dxbc_spv::util::ByteReader code, bool isSWVP);

  bool runAnalysis();

  ShaderInfo getShaderInfo() const {
    return m_parser.getShaderInfo();
  }

  size_t getLength() const {
    return m_length;
  }

  const ShaderConstantsInfo& getConstantsInfo() const {
    return m_constants;
  }

  const ImmediateConstants& getImmediateConstants() const {
    return m_immediateConstants;
  }

  RenderTargetMask getRenderTargetMask() const {
    return m_usedRTs;
  }

  SamplerMask getSamplerMask() const {
    return m_usedSamplers;
  }

  TextureType getTextureType(uint32_t index) const {
    return m_textureTypes[index];
  }

  bool isSWVP() const {
    return m_isSWVP;
  }

  uint32_t getInputSignatureSize() const {
    return m_inputSignature.size();
  }

  InputSignatureElement getInputSignatureElement(uint32_t index) const {
    return m_inputSignature[index];
  }

private:

  bool initParser(Parser& parser, dxbc_spv::util::ByteReader reader);

  bool handleInstruction(const Instruction& op);

  bool handleDef(const Instruction& op);

  bool handleTextureSample(const Instruction& op);

  bool handleDcl(const Instruction& op);

  dxbc_spv::util::ByteReader m_code;

  Parser           m_parser;

  bool             m_isSWVP;

  size_t m_length = 0u;

  ShaderConstantsInfo m_constants;

  ImmediateConstants m_immediateConstants;

  RenderTargetMask m_usedRTs = 0u;

  SamplerMask m_usedSamplers = 0u;

  std::array<TextureType, 16u> m_textureTypes = {};

  Signature m_inputSignature    = {};

};

}
