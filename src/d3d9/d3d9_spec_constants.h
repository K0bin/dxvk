#pragma once

#include <array>

#include <cstdint>

#include "../spirv/spirv_module.h"

class D3D9DeviceEx;

namespace dxvk {

  enum D3D9SpecConstantId : uint32_t {
    SpecSamplerType,        // 2 bits for 16 PS samplers      | Bits: 32

    SpecSamplerDepthMode,   // 1 bit for 21 VS + PS samplers  | Bits: 21
    SpecAlphaCompareOp,     // Range: 0 -> 7                  | Bits: 3
    SpecPointMode,          // Range: 0 -> 3                  | Bits: 2
    SpecVertexFogMode,      // Range: 0 -> 3                  | Bits: 2
    SpecPixelFogMode,       // Range: 0 -> 3                  | Bits: 2
    SpecFogEnabled,         // Range: 0 -> 1                  | Bits: 1

    SpecSamplerNull,        // 1 bit for 21 * 5 samplers      | Bits: 21
    SpecProjectionType,     // 1 bit for 6 PS 1.x samplers    | Bits: 6
    SpecAlphaPrecisionBits, // Range: 0 -> 8 or 0xF           | Bits: 4

    SpecVertexShaderBools,  // 16 bools                       | Bits: 16
    SpecPixelShaderBools,   // 16 bools                       | Bits: 16

    SpecDrefClamp,          // 1 bit for 16 PS samplers       | Bits: 16
    SpecFetch4,             // 1 bit for 16 PS samplers       | Bits: 16

    SpecConstantCount,
  };

  struct BitfieldPosition {
    constexpr uint32_t mask() const {
      return uint32_t((1ull << sizeInBits) - 1) << bitOffset;
    }

    uint32_t dwordOffset;
    uint32_t bitOffset;
    uint32_t sizeInBits;
  };

  struct D3D9SpecializationInfo {
    static constexpr uint32_t MaxSpecDwords = 5;

    static constexpr std::array<BitfieldPosition, SpecConstantCount> Layout{{
      { 0, 0, 32 },  // SamplerType
      
      { 1, 0,  21 }, // SamplerDepthMode
      { 1, 21, 3 },  // AlphaCompareOp
      { 1, 24, 2 },  // PointMode
      { 1, 26, 2 },  // VertexFogMode
      { 1, 28, 2 },  // PixelFogMode
      { 1, 30, 1 },  // FogEnabled

      { 2, 0,  105 }, // SamplerNull
      { 5, 9, 6 },  // ProjectionType
      { 5, 15, 4 },  // AlphaPrecisionBits

      { 5, 19,  16 }, // VertexShaderBools
      { 6, 4, 16 }, // PixelShaderBools

      { 6, 20,  16 }, // DrefClamp
      { 7, 112, 16 }, // Fetch4
    }};

    template <D3D9SpecConstantId Id, typename T>
    bool set(const T& value) {
      const uint32_t x = uint32_t(value);
      if (get<Id>() == x)
        return false;

      constexpr auto& layout = Layout[Id];

      data[layout.dwordOffset] &= ~layout.mask();
      data[layout.dwordOffset] |= (x << layout.bitOffset) & layout.mask();

      return true;
    }

    template <D3D9SpecConstantId Id>
    uint32_t get() const {
      constexpr auto& layout = Layout[Id];

      return (data[layout.dwordOffset] & layout.mask()) >> layout.bitOffset;
    }

    std::array<uint32_t, MaxSpecDwords> data = {};
  };

  class D3D9ShaderSpecConstantManager {
  public:
    uint32_t get(SpirvModule &module, D3D9SpecConstantId id) {
      return get(module, id, 0, 32);
    }

    uint32_t get(SpirvModule &module, D3D9SpecConstantId id, uint32_t bitOffset, uint32_t bitCount) {
      const auto &layout = D3D9SpecializationInfo::Layout[id];

      uint32_t val = getSpecConstDword(module, layout.dwordOffset);
      bitCount = std::min(bitCount, layout.sizeInBits - bitOffset);

      if (bitCount == 32)
        return val;

      return module.opBitFieldUExtract(
        module.defIntType(32, 0), val,
        module.consti32(bitOffset + layout.bitOffset),
        module.consti32(bitCount));
    }

  private:
    uint32_t getSpecConstDword(SpirvModule &module, uint32_t idx) {
      if (!m_specConstantIds[idx]) {
        m_specConstantIds[idx] = module.specConst32(module.defIntType(32, 0), 0);
        module.decorateSpecId(m_specConstantIds[idx], idx);
      }

      return m_specConstantIds[idx];
    }

    std::array<uint32_t, MaxNumSpecConstants> m_specConstantIds = {};
  };

}
