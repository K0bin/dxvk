#pragma once

#include "../util/util_vector.h"

#include <util/util_byte_stream.h>
#include <util/util_small_vector.h>

#include <sm3/sm3_types.h>
#include <sm3/sm3_parser.h>
#include <sm3/sm3_resources.h>

#include <vulkan/vulkan.h>

#include <vector>

using namespace dxbc_spv::sm3;
using dxbc_spv::util::small_vector;

namespace dxvk {

struct D3D9ImmediateFloatConstant {
  uint32_t index;
  Vector4 value;
};

using D3D9ImmediateFloatConstants = std::vector<D3D9ImmediateFloatConstant>;

struct D3D9ImmediateConstants {
  int32_t maxFloatIndex = -1;
  int32_t maxIntIndex   = -1;
  int32_t maxBoolIndex  = -1;

  D3D9ImmediateFloatConstants floats;
};

struct D3D9ShaderConstantsInfo {
  bool floatsAccessedDynamically = false;

  int32_t maxFloatIndex = -1;
  int32_t maxIntIndex   = -1;
  int32_t maxBoolIndex  = -1;

  uint32_t boolMask = 0u;
};

struct D3D9InputSignatureElement {
  uint32_t location;
  Semantic semantic;
};

using D3D9RenderTargetMask = uint8_t;
using D3D9SamplerMask      = uint32_t;
using D3D9InputSignature   = small_vector<D3D9InputSignatureElement, 16u>;

class D3D9ShaderAnalysis {

public:

  D3D9ShaderAnalysis() = default;

  D3D9ShaderAnalysis(dxbc_spv::util::ByteReader code, bool isSWVP);

  D3D9ShaderAnalysis(D3D9ShaderAnalysis&& other);

  D3D9ShaderAnalysis(const D3D9ShaderAnalysis& other);

  D3D9ShaderAnalysis& operator=(const D3D9ShaderAnalysis& other);

  D3D9ShaderAnalysis& operator=(D3D9ShaderAnalysis&& other);

  ShaderInfo GetShaderInfo() const {
    return m_shaderInfo;
  }

  size_t GetLength() const {
    return m_length;
  }

  const D3D9ShaderConstantsInfo& GetConstantsInfo() const {
    return m_constants;
  }

  const D3D9ImmediateConstants& GetImmediateConstants() const {
    return m_immediateConstants;
  }

  D3D9RenderTargetMask GetRenderTargetMask() const {
    return m_usedRTs;
  }

  D3D9SamplerMask GetSamplerMask() const {
    return m_usedSamplers;
  }

  VkImageViewType GetImageViewType(uint32_t index) const {
    return m_imageViewTypes[index];
  }

  bool IsSWVP() const {
    return m_isSWVP;
  }

  uint32_t GetInputSignatureSize() const {
    return m_inputSignature.size();
  }

  D3D9InputSignatureElement GetInputSignatureElement(uint32_t index) const {
    return m_inputSignature[index];
  }

  const D3D9InputSignature& GetInputSignature() const { return m_inputSignature; }

  uint32_t GetFlatShadingMask() const {
    return m_flatShadingMask;
  }

private:

  bool RunAnalysis(Parser& parser);

  bool HandleInstruction(const Instruction& op);

  bool HandleDef(const Instruction& op);

  bool HandleTextureSample(const Instruction& op);

  bool HandleDcl(const Instruction& op);

  bool             m_isSWVP = false;

  uint32_t m_length = 0u;

  ShaderInfo m_shaderInfo;

  D3D9ShaderConstantsInfo m_constants;

  D3D9ImmediateConstants m_immediateConstants;

  D3D9RenderTargetMask m_usedRTs = 0u;

  D3D9SamplerMask m_usedSamplers = 0u;

  std::array<VkImageViewType, 16u> m_imageViewTypes = {};

  uint32_t m_flatShadingMask = 0u;

  D3D9InputSignature m_inputSignature    = {};

};

}
