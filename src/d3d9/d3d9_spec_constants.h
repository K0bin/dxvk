#pragma once

#include <array>

#include <cstdint>

#include "../spirv/spirv_module.h"

class D3D9DeviceEx;

namespace dxvk {

  enum D3D9SpecConstantId : uint32_t {
    SpecSamplerType,        // 2 bits for 16 PS samplers      | Bits: 32

    SpecSamplerIsDepth,     // 1 bit for 21 VS + PS samplers  | Bits: 21
    SpecAlphaCompareOp,     // Range: 0 -> 7                  | Bits: 3
    SpecProjected,          // 1 bit for 6 PS 1.x samplers    | Bits: 8
                            // (1 bit for 8 FF texture stages)

    SpecSamplerIsNull,      // 1 bit for 21 samplers          | Bits: 21
    SpecAlphaPrecisionBits, // Range: 0 -> 8 or 0xF           | Bits: 4
    SpecFogEnabled,         // Range: 0 -> 1                  | Bits: 1
    SpecVertexFogMode,      // Range: 0 -> 3                  | Bits: 2
    SpecPixelFogMode,       // Range: 0 -> 3                  | Bits: 2

    SpecVertexShaderBools,  // 16 bools                       | Bits: 16
    SpecPixelShaderBools,   // 16 bools                       | Bits: 16

    SpecFetch4,             // 1 bit for 16 PS samplers       | Bits: 16

    SpecDrefClamp,          // 1 bit for 21 VS + PS samplers  | Bits: 21
    SpecDrefScaling,        // Range: 0 -> 31                 | Bits: 5
    SpecClipPlaneCount,     // Range: 0 ->                    | Bits: 3
    SpecPointMode,          // Range: 0 -> 3                  | Bits: 2

    // Fixed function state
    // VS
    SpecFFTexcoordIndices,    // 3 bits for 8 texture stages | Bits: 24
    SpecFFVertexHasPositionT, // 1 bool                      | Bits: 1
    SpecFFVertexHasColor0,    // 1 bool                      | Bits: 1
    SpecFFVertexHasColor1,    // 1 bool                      | Bits: 1
    SpecFFVertexHasPointSize, // 1 bool                      | Bits: 1
    SpecFFUseLighting,        // 1 bool                      | Bits: 1
    SpecFFNormalizeNormals,   // 1 bool                      | Bits: 1
    SpecFFLocalViewer,        // 1 bool                      | Bits: 1
    SpecFFRangeFog,           // 1 bool                      | Bits: 1
    SpecFFTexcoordFlags,      // 3 bits for 8 texture stages | Bits: 24
    SpecFFDiffuseSource,      // Range: 0 -> 2               | Bits: 2
    SpecFFAmbientSource,      // Range: 0 -> 2               | Bits: 2
    SpecFFSpecularSource,     // Range: 0 -> 2               | Bits: 2
    SpecFFEmissiveSource,     // Range: 0 -> 2               | Bits: 2
    SpecFFTransformFlags,     // 3 bits for 8 texture stages | Bits: 24
    SpecFFVertexBlendMode,    // Range: 0 -> 2               | Bits: 2
    SpecFFBlendVertexIndexed, // 1 bool                      | Bits: 1
    SpecFFVertexBlendCount,   // Range: 0 -> 3               | Bits: 2
    SpecFFVertexClipping,     // 1 bool                      | Bits: 1

    // FS
    SpecFFTextureStage0ColorOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage0ColorArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage0ColorArg1,    // Same as above        | Bits 6
    SpecFFTextureStage0ColorArg2,    // Same as above        | Bits 6
    SpecFFTextureStage0AlphaOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage0AlphaArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage0AlphaArg1,    // Same as above        | Bits 6
    SpecFFTextureStage0AlphaArg2,    // Same as above        | Bits 6
    SpecFFTextureStage0ResultIsTemp, // Range 0 - 3          | Bits 2

    SpecFFTextureStage1ColorOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage1ColorArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage1ColorArg1,    // Same as above        | Bits 6
    SpecFFTextureStage1ColorArg2,    // Same as above        | Bits 6
    SpecFFTextureStage1AlphaOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage1AlphaArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage1AlphaArg1,    // Same as above        | Bits 6
    SpecFFTextureStage1AlphaArg2,    // Same as above        | Bits 6
    SpecFFTextureStage1ResultIsTemp, // Range 0 - 3          | Bits 2

    SpecFFTextureStage2ColorOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage2ColorArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage2ColorArg1,    // Same as above        | Bits 6
    SpecFFTextureStage2ColorArg2,    // Same as above        | Bits 6
    SpecFFTextureStage2AlphaOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage2AlphaArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage2AlphaArg1,    // Same as above        | Bits 6
    SpecFFTextureStage2AlphaArg2,    // Same as above        | Bits 6
    SpecFFTextureStage2ResultIsTemp, // Range 0 - 3          | Bits 2

    SpecFFTextureStage3ColorOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage3ColorArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage3ColorArg1,    // Same as above        | Bits 6
    SpecFFTextureStage3ColorArg2,    // Same as above        | Bits 6
    SpecFFTextureStage3AlphaOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage3AlphaArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage3AlphaArg1,    // Same as above        | Bits 6
    SpecFFTextureStage3AlphaArg2,    // Same as above        | Bits 6
    SpecFFTextureStage3ResultIsTemp, // Range 0 - 3          | Bits 2

    SpecFFTextureStage4ColorOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage4ColorArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage4ColorArg1,    // Same as above        | Bits 6
    SpecFFTextureStage4ColorArg2,    // Same as above        | Bits 6
    SpecFFTextureStage4AlphaOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage4AlphaArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage4AlphaArg1,    // Same as above        | Bits 6
    SpecFFTextureStage4AlphaArg2,    // Same as above        | Bits 6
    SpecFFTextureStage4ResultIsTemp, // Range 0 - 3          | Bits 2

    SpecFFTextureStage5ColorOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage5ColorArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage5ColorArg1,    // Same as above        | Bits 6
    SpecFFTextureStage5ColorArg2,    // Same as above        | Bits 6
    SpecFFTextureStage5AlphaOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage5AlphaArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage5AlphaArg1,    // Same as above        | Bits 6
    SpecFFTextureStage5AlphaArg2,    // Same as above        | Bits 6
    SpecFFTextureStage5ResultIsTemp, // Range 0 - 3          | Bits 2

    SpecFFTextureStage6ColorOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage6ColorArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage6ColorArg1,    // Same as above        | Bits 6
    SpecFFTextureStage6ColorArg2,    // Same as above        | Bits 6
    SpecFFTextureStage6AlphaOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage6AlphaArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage6AlphaArg1,    // Same as above        | Bits 6
    SpecFFTextureStage6AlphaArg2,    // Same as above        | Bits 6
    SpecFFTextureStage6ResultIsTemp, // Range 0 - 3          | Bits 2

    SpecFFTextureStage7ColorOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage7ColorArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage7ColorArg1,    // Same as above        | Bits 6
    SpecFFTextureStage7ColorArg2,    // Same as above        | Bits 6
    SpecFFTextureStage7AlphaOp,      // Range 1 -> 26        | Bits 5
    SpecFFTextureStage7AlphaArg0,    // Range 0 -> 5         | Bits 6
                                     // + 2 modifier bits
    SpecFFTextureStage7AlphaArg1,    // Same as above        | Bits 6
    SpecFFTextureStage7AlphaArg2,    // Same as above        | Bits 6
    SpecFFTextureStage7ResultIsTemp, // Range 0 - 3          | Bits 2

    SpecFFGlobalSpecularEnable,      // Bool                 | Bits 1

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
    // Spec const word 0 determines whether the other spec constants are used rather than the spec const UBO
    static constexpr uint32_t MaxSpecDwords = 23;

    static constexpr uint32_t MaxUBODwords  = 23;
    static constexpr size_t UBOSize = MaxUBODwords * sizeof(uint32_t);

    static constexpr std::array<BitfieldPosition, SpecConstantCount> Layout{{
      { 0, 0, 32 },  // SamplerType

      { 1, 0,  21 }, // SamplerIsDepth
      { 1, 21, 3 },  // AlphaCompareOp
      { 1, 24, 8 },  // Projected

      { 2, 0,  21 }, // SamplerIsNull
      { 2, 21, 4 },  // AlphaPrecisionBits
      { 1, 25, 1 },  // FogEnabled
      { 2, 26, 2 },  // VertexFogMode
      { 2, 28, 2 },  // PixelFogMode

      { 3, 0,  16 }, // VertexShaderBools
      { 3, 16, 16 }, // PixelShaderBools

      { 4, 0,  16 }, // Fetch4

      { 5, 0, 21 },  // DrefClamp
      { 5, 21, 5 },  // DrefScaling
      { 5, 26, 3 },  // ClipPlaneCount
      { 5, 29, 2 },  // PointMode

      { 6, 0, 24 },  // SpecFFTexcoordIndices
      { 6, 24, 1 },  // SpecFFVertexHasPositionT
      { 6, 25, 1 },  // SpecFFVertexHasColor0
      { 6, 26, 1 },  // SpecFFVertexHasColor1
      { 6, 27, 1 },  // SpecFFVertexHasPointSize
      { 6, 28, 1 },  // SpecFFUseLighting
      { 6, 29, 1 },  // SpecFFNormalizeNormals
      { 6, 30, 1 },  // SpecFFLocalViewer
      { 6, 31, 1 },  // SpecFFRangeFog
      { 7, 0, 24 },  // SpecFFTexcoordFlags
      { 7, 24, 2 },  // SpecFFDiffuseSource
      { 7, 26, 2 },  // SpecFFAmbientSource
      { 7, 28, 2 },  // SpecFFSpecularSource
      { 7, 30, 2 },  // SpecFFEmissiveSource
      { 8, 0, 24 },  // SpecFFTransformFlags
      { 8, 24, 2 },  // SpecFFVertexBlendMode
      { 8, 26, 1 },  // SpecFFBlendVertexIndexed
      { 8, 27, 2 },  // SpecFFVertexBlendCount
      { 8, 29, 1 },  // SpecFFVertexClipping

      { 9, 0, 5 },   // SpecFFTextureStage0ColorOp
      { 9, 5, 6 },   // SpecFFTextureStage0ColorArg0
      { 9, 11, 6 },  // SpecFFTextureStage0ColorArg1
      { 9, 17, 6 },  // SpecFFTextureStage0ColorArg2
      { 9, 23, 5 },  // SpecFFTextureStage0AlphaOp
      { 10, 0, 6 },  // SpecFFTextureStage0AlphaArg0
      { 10, 6, 6 },  // SpecFFTextureStage0AlphaArg1
      { 10, 12, 6 }, // SpecFFTextureStage0AlphaArg2
      { 10, 18, 1 }, // SpecFFTextureStage0ResultIsTemp

      { 10, 19, 5 }, // SpecFFTextureStage1ColorOp
      { 10, 24, 6 }, // SpecFFTextureStage1ColorArg0
      { 11, 0, 6 },  // SpecFFTextureStage1ColorArg1
      { 11, 6, 6 },  // SpecFFTextureStage1ColorArg2
      { 11, 12, 5 }, // SpecFFTextureStage1AlphaOp
      { 11, 17, 6 }, // SpecFFTextureStage1AlphaArg0
      { 11, 23, 6 }, // SpecFFTextureStage1AlphaArg1
      { 12, 0, 6 },  // SpecFFTextureStage1AlphaArg2
      { 12, 6, 1 },  // SpecFFTextureStage1ResultIsTemp

      { 12, 7, 5 },  // SpecFFTextureStage2ColorOp
      { 12, 12, 6 }, // SpecFFTextureStage2ColorArg0
      { 12, 18, 6 }, // SpecFFTextureStage2ColorArg1
      { 12, 24, 6 }, // SpecFFTextureStage2ColorArg2
      { 13, 0, 5 },  // SpecFFTextureStage2AlphaOp
      { 13, 5, 6 },  // SpecFFTextureStage2AlphaArg0
      { 13, 11, 6 }, // SpecFFTextureStage2AlphaArg1
      { 13, 17, 6 }, // SpecFFTextureStage2AlphaArg2
      { 13, 23, 1 }, // SpecFFTextureStage2ResultIsTemp

      { 13, 24, 5 }, // SpecFFTextureStage3ColorOp
      { 14, 0, 6 },  // SpecFFTextureStage3ColorArg0
      { 14, 6, 6 },  // SpecFFTextureStage3ColorArg1
      { 14, 12, 6 }, // SpecFFTextureStage3ColorArg2
      { 14, 18, 5 }, // SpecFFTextureStage3AlphaOp
      { 14, 23, 6 }, // SpecFFTextureStage3AlphaArg0
      { 15, 0, 6 },  // SpecFFTextureStage3AlphaArg1
      { 15, 23, 6 }, // SpecFFTextureStage3AlphaArg2
      { 15, 29, 1 }, // SpecFFTextureStage3ResultIsTemp

      { 16, 0, 5 },  // SpecFFTextureStage4ColorOp
      { 16, 5, 6 },  // SpecFFTextureStage4ColorArg0
      { 16, 11, 6 }, // SpecFFTextureStage4ColorArg1
      { 16, 17, 6 }, // SpecFFTextureStage4ColorArg2
      { 16, 23, 5 }, // SpecFFTextureStage4AlphaOp
      { 17, 0, 6 },  // SpecFFTextureStage4AlphaArg0
      { 17, 6, 6 },  // SpecFFTextureStage4AlphaArg1
      { 17, 12, 6 }, // SpecFFTextureStage4AlphaArg2
      { 17, 18, 1 }, // SpecFFTextureStage4ResultIsTemp

      { 17, 19, 5 }, // SpecFFTextureStage3ColorOp
      { 17, 24, 6 }, // SpecFFTextureStage3ColorArg0
      { 18, 0, 6 },  // SpecFFTextureStage3ColorArg1
      { 18, 6, 6 },  // SpecFFTextureStage3ColorArg2
      { 18, 12, 5 }, // SpecFFTextureStage3AlphaOp
      { 18, 17, 6 }, // SpecFFTextureStage3AlphaArg0
      { 18, 23, 6 }, // SpecFFTextureStage3AlphaArg1
      { 19, 0, 6 },  // SpecFFTextureStage3AlphaArg2
      { 19, 6, 1 },  // SpecFFTextureStage3ResultIsTemp

      { 19, 7, 5 },  // SpecFFTextureStage6ColorOp
      { 19, 12, 6 }, // SpecFFTextureStage6ColorArg0
      { 19, 18, 6 }, // SpecFFTextureStage6ColorArg1
      { 19, 24, 6 }, // SpecFFTextureStage6ColorArg2
      { 20, 0, 5 },  // SpecFFTextureStage6AlphaOp
      { 20, 5, 6 },  // SpecFFTextureStage6AlphaArg0
      { 20, 11, 6 }, // SpecFFTextureStage6AlphaArg1
      { 20, 17, 6 }, // SpecFFTextureStage6AlphaArg2
      { 20, 23, 1 }, // SpecFFTextureStage6ResultIsTemp

      { 20, 24, 5 }, // SpecFFTextureStage7ColorOp
      { 21, 0, 6 },  // SpecFFTextureStage7ColorArg0
      { 21, 6, 6 },  // SpecFFTextureStage7ColorArg1
      { 21, 12, 6 }, // SpecFFTextureStage7ColorArg2
      { 21, 18, 5 }, // SpecFFTextureStage7AlphaOp
      { 21, 23, 6 }, // SpecFFTextureStage7AlphaArg0
      { 22, 0, 6 }, // SpecFFTextureStage7AlphaArg1
      { 22, 6, 6 }, // SpecFFTextureStage7AlphaArg2
      { 22, 18, 1 }, // SpecFFTextureStage7ResultIsTemp

      { 22, 19, 1 }, // SpecFFGlobalSpecularEnable
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

    template <typename T>
    bool setDynamic(D3D9SpecConstantId id, const T& value) {
      const uint32_t x = uint32_t(value);
      if (getDynamic(id) == x)
        return false;

      const auto& layout = Layout[id];

      data[layout.dwordOffset] &= ~layout.mask();
      data[layout.dwordOffset] |= (x << layout.bitOffset) & layout.mask();

      return true;
    }

    uint32_t getDynamic(D3D9SpecConstantId id) const {
      const auto& layout = Layout[id];

      return (data[layout.dwordOffset] & layout.mask()) >> layout.bitOffset;
    }

    template <D3D9SpecConstantId Id, typename T>
    bool setDword(T dwordValue) {
      constexpr auto& layout = Layout[Id];
      const uint32_t x = uint32_t(dwordValue);
      if (data[layout.dwordOffset] == x)
        return false;

      data[layout.dwordOffset] = x;
      return true;
    }

    template <typename T>
    bool setDwordDynamic(D3D9SpecConstantId id, T dwordValue) {
      const auto& layout = Layout[id];
      const uint32_t x = uint32_t(dwordValue);
      if (data[layout.dwordOffset] == x)
        return false;

      data[layout.dwordOffset] = x;
      return true;
    }

    std::array<uint32_t, MaxSpecDwords> data = {};
  };

  class D3D9ShaderSpecConstantManager {
  public:
    uint32_t get(SpirvModule &module, uint32_t specUbo, D3D9SpecConstantId id) {
      return get(module, specUbo, id, 0, 32);
    }

    uint32_t get(SpirvModule &module, uint32_t specUbo, D3D9SpecConstantId id, uint32_t bitOffset, uint32_t bitCount, uint32_t uboOverride = 0) {
      const auto &layout = D3D9SpecializationInfo::Layout[id];

      uint32_t uintType = module.defIntType(32, 0);
      uint32_t optimized = getOptimizedBool(module);

      uint32_t quickValue     = uboOverride ? uboOverride : getSpecUBODword(module, specUbo, layout.dwordOffset);
      uint32_t optimizedValue = getSpecConstDword(module, layout.dwordOffset);

      uint32_t val = module.opSelect(uintType, optimized, optimizedValue, quickValue);
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

    uint32_t getSpecUBODword(SpirvModule& module, uint32_t specUbo, uint32_t idx) {
      uint32_t uintType = module.defIntType(32, 0);
      uint32_t uintPtr  = module.defPointerType(uintType, spv::StorageClassUniform);

      uint32_t member = module.constu32(idx);
      uint32_t dword  = module.opLoad(uintType, module.opAccessChain(uintPtr, specUbo, 1, &member));

      return dword;
    }

    uint32_t getOptimizedBool(SpirvModule& module) {
      uint32_t boolType = module.defBoolType();

      // The spec constant at MaxNumSpecConstants is set to True
      // when this is an optimized pipeline.
      uint32_t optimized = getSpecConstDword(module, MaxNumSpecConstants);
      optimized = module.opINotEqual(boolType, optimized, module.constu32(0));

      return optimized;
    }

    std::array<uint32_t, MaxNumSpecConstants + 1> m_specConstantIds = {};
  };

}