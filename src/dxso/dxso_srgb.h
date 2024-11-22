#pragma once

#include "../spirv/spirv_module.h"
#include "../d3d9/d3d9_caps.h"
#include "../d3d9/d3d9_spec_constants.h"

namespace dxvk {

  static void emitSrgbConversion(
    SpirvModule& module,
    D3D9ShaderSpecConstantManager& spec,
    uint32_t specUbo,
    const std::array<uint32_t, caps::MaxSimultaneousRenderTargets>& rtRegIds
  ) {
    uint32_t boolTypeId = module.defBoolType();
    uint32_t floatTypeId = module.defFloatType(32);
    uint32_t vec4fTypeId = module.defVectorType(floatTypeId, 3);
    uint32_t vec4bTypeId = module.defVectorType(boolTypeId, 4);

    uint32_t specConstVal = spec.get(module, specUbo, SpecSrgb, 0, 1);
    uint32_t doSrgb = module.opIEqual(boolTypeId, specConstVal, module.constu32(1u));
    const std::array<uint32_t, 4> repeatSrgb = { doSrgb, doSrgb, doSrgb, module.constBool(false) /* Always get alpha from the original vector*/ };
    uint32_t doSrgbVec = module.opCompositeConstruct(vec4bTypeId, 4, repeatSrgb.data());

    for (uint32_t i = 0; i < caps::MaxSimultaneousRenderTargets; i++) {
      if (rtRegIds[i] == 0)
        // Shader doesn't write RT i
        continue;

      /*
      Adapted from:
      vec3 linear_to_srgb(vec3 linear) {
        bvec3 isLo = lessThanEqual(linear, vec3(0.0031308f));

        vec3 loPart = linear * 12.92f;
        vec3 hiPart = pow(linear, vec3(5.0f / 12.0f)) * 1.055f - 0.055f;
        return mix(hiPart, loPart, isLo);
      }
      We do however do all the math on vec4s to avoid having to convert it to vec3 and back.
      At the end it will just take the alpha from the original RT register anyway.
      */

      // Load linear render result
      uint32_t linear = module.opLoad(vec4fTypeId, rtRegIds[i]);

      const uint32_t lo = module.constvec4f32(0.0031308f, 0.0031308f, 0.0031308f, 0.0f);
      uint32_t isLo = module.opFOrdLessThanEqual(vec4bTypeId, linear, lo);

      uint32_t factorConstId = module.constvec4f32(12.92f, 12.92f, 12.92f, 1.0f);
      uint32_t loPart = module.opFMul(vec4fTypeId, linear, factorConstId);

      uint32_t hiPart = module.opPow(vec4fTypeId, linear, module.constvec4f32(5.0f / 12.0f, 5.0f / 12.0f, 5.0f / 12.0f, 1.0f));
      hiPart = module.opFMul(vec4fTypeId, hiPart, module.constvec4f32(1.055f, 1.055f, 1.055f, 1.0f));
      hiPart = module.opFSub(vec4fTypeId, hiPart, module.constvec4f32(0.055f, 0.055f, 0.055f, 0.0f));

      uint32_t srgb = module.opSelect(vec4fTypeId, isLo, loPart, hiPart);

      // Decide whether we use the SRGB vector or the original linear one
      uint32_t finalRTOutput = module.opSelect(vec4fTypeId, doSrgbVec, srgb, linear);

      module.opStore(rtRegIds[i], finalRTOutput);
    }
  }

}
