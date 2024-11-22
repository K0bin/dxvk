#pragma once

#include "../spirv/spirv_module.h"
#include "../d3d9/d3d9_caps.h"
#include "../d3d9/d3d9_spec_constants.h"

namespace dxvk {

  void emitSrgbConversion(
    SpirvModule& module,
    D3D9ShaderSpecConstantManager& spec,
    uint32_t specUbo,
    const std::array<uint32_t, caps::MaxSimultaneousRenderTargets>& rtRegIds
  ) {
    uint32_t boolTypeId = module.defBoolType();
    uint32_t floatTypeId = module.defFloatType(32);
    uint32_t vec3fTypeId = module.defVectorType(floatTypeId, 3);
    uint32_t vec3bTypeId = module.defVectorType(boolTypeId, 3);
    uint32_t vec4fTypeId = module.defVectorType(floatTypeId, 3);
    uint32_t vec4bTypeId = module.defVectorType(boolTypeId, 4);

    uint32_t specConstVal = spec.get(module, specUbo, SpecSrgb, 0, 1);
    uint32_t doSrgb = module.opIEqual(boolTypeId, specConstVal, module.constu32(1u));
    const std::array<uint32_t, 4> repeatSrgb = { doSrgb, doSrgb, doSrgb, doSrgb };
    uint32_t doSrgbVec = module.opCompositeConstruct(vec4bTypeId, 4, repeatSrgb.data());

    for (uint32_t i = 0; i < caps::MaxSimultaneousRenderTargets; i++) {
      if (rtRegIds[i] == 0)
        // Shader doesn't write RT i
        continue;

      uint32_t linearVal = module.opLoad(vec4fTypeId, rtRegIds[i]);

      const std::array<uint32_t, 3> indices = { 0, 1, 2 };
      uint32_t linearVec3 = module.opVectorShuffle(vec3fTypeId, linearVal, linearVal, 3, indices.data());

      const uint32_t lo = module.constvec3f32(0.0031308f, 0.0031308f, 0.0031308f);
      uint32_t isLo = module.opFOrdLessThanEqual(vec3bTypeId, linearVec3, lo);

      uint32_t factorConstId = module.constvec3f32(12.92f, 12.92f, 12.92f);
      uint32_t loPart = module.opFMul(vec3fTypeId, linearVec3, factorConstId);

      uint32_t hiPart = module.opPow(vec3fTypeId, linearVec3, module.constvec3f32(5.0f / 12.0f, 5.0f / 12.0f, 5.0f / 12.0f));
      hiPart = module.opFMul(vec3fTypeId, hiPart, module.constvec3f32(1.055f, 1.055f, 1.055f));
      hiPart = module.opFSub(vec3fTypeId, hiPart, module.constvec3f32(0.055f, 0.055f, 0.055f));

      uint32_t srgbVec3 = module.opSelect(vec3fTypeId, isLo, loPart, hiPart);
      const std::array<uint32_t, 4> finalVec4Indices = {0, 1, 2, 3};
      uint32_t srgbVec4 = module.opVectorShuffle(vec4fTypeId, srgbVec3, linearVal, 4, finalVec4Indices.data());

      uint32_t finalRTOutput = module.opSelect(vec4fTypeId, doSrgbVec, srgbVec4, linearVal);

      module.opStore(rtRegIds[i], finalRTOutput);
    }
  }

}
