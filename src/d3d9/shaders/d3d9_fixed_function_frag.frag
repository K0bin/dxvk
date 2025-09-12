#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_spirv_intrinsics : require
#extension GL_EXT_demote_to_helper_invocation : require
#extension GL_ARB_derivative_control : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_nonuniform_qualifier : require

#define GLSL
#include "../d3d9_shader_types.h"

const uint TextureStageCount = 8;
const uint SpecConstOptimizedTextureStageCount = 4;
const uint TextureArgCount = 3;

// The locations need to match with RegisterLinkerSlot in dxso_util.cpp
layout(location = 0) in vec4 in_Normal;
layout(location = 1) in vec4 in_Texcoord0;
layout(location = 2) in vec4 in_Texcoord1;
layout(location = 3) in vec4 in_Texcoord2;
layout(location = 4) in vec4 in_Texcoord3;
layout(location = 5) in vec4 in_Texcoord4;
layout(location = 6) in vec4 in_Texcoord5;
layout(location = 7) in vec4 in_Texcoord6;
layout(location = 8) in vec4 in_Texcoord7;
layout(location = 9) in vec4 in_Color0;
layout(location = 10) in vec4 in_Color1;
layout(location = 11) in float in_Fog;

layout(location = 0) out vec4 out_Color0;

// Bindings have to match with computeResourceSlotId in dxso_util.h
// computeResourceSlotId(
//     DxsoProgramType::PixelShader,
//     DxsoBindingType::ConstantBuffer,
//     DxsoConstantBuffers::PSFixedFunction
// ) = 11
layout(set = 0, binding = 11, scalar, row_major) uniform ShaderData {
    D3D9FixedFunctionPS data;
};

// Bindings have to match with computeResourceSlotId in dxso_util.h
// computeResourceSlotId(
//     DxsoProgramType::PixelShader,
//     DxsoBindingType::ConstantBuffer,
//     DxsoConstantBuffers::PSShared
// ) = 12
layout(set = 0, binding = 12, scalar, row_major) uniform SharedData {
    D3D9SharedPS sharedData;
};

layout(push_constant, scalar, row_major) uniform RenderStates {
    D3D9RenderStateInfo rs;
    uint packedSamplerIndices[4];
};


// Dynamic "spec constants"
// See d3d9_spec_constants.h for packing
// Binding has to match with getSpecConstantBufferSlot in dxso_util.h
layout(set = 0, binding = 31, scalar) uniform SpecConsts {
    uint dynamicSpecConstDword[13];
};


layout(set = 0, binding = 13) uniform texture2D t2d[8];
layout(set = 0, binding = 13) uniform textureCube tcube[8];
layout(set = 0, binding = 13) uniform texture3D t3d[8];


layout(origin_upper_left) in vec4 gl_FragCoord;


layout(set = 15, binding = 0) uniform sampler sampler_heap[];



// Thanks SPIRV-Cross
spirv_instruction(set = "GLSL.std.450", id = 79) float spvNMin(float, float);
spirv_instruction(set = "GLSL.std.450", id = 79) vec2 spvNMin(vec2, vec2);
spirv_instruction(set = "GLSL.std.450", id = 79) vec3 spvNMin(vec3, vec3);
spirv_instruction(set = "GLSL.std.450", id = 79) vec4 spvNMin(vec4, vec4);
spirv_instruction(set = "GLSL.std.450", id = 81) float spvNClamp(float, float, float);
spirv_instruction(set = "GLSL.std.450", id = 81) vec2 spvNClamp(vec2, vec2, vec2);
spirv_instruction(set = "GLSL.std.450", id = 81) vec3 spvNClamp(vec3, vec3, vec3);
spirv_instruction(set = "GLSL.std.450", id = 81) vec4 spvNClamp(vec4, vec4, vec4);


// Functions to extract information from the packed texture stages
// See D3D9FFTextureStage in d3d9_shader_types.h
// Please, dearest compiler, inline all of this.
uint colorOp(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 0, 5);
}
uint colorArg0(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 5, 6);
}
uint colorArg1(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 11, 6);
}
uint colorArg2(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 17, 6);
}

uint alphaOp(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 23, 5);
}
uint alphaArg0(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 0, 6);
}
uint alphaArg1(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 6, 6);
}
uint alphaArg2(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 12, 6);
}

bool resultIsTemp(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 18, 1) != 0;
}
bool globalSpecularEnabled() {
    return bitfieldExtract(data.Stages[0].Primitive[1], 19, 1) != 0;
}


// Functions to extract information from the packed dynamic spec consts
// See d3d9_spec_constants.h for packing
// Please, dearest compiler, inline all of this.
layout (constant_id = 0) const uint SpecConstDword0 = 0;
layout (constant_id = 1) const uint SpecConstDword1 = 0;
layout (constant_id = 2) const uint SpecConstDword2 = 0;
layout (constant_id = 3) const uint SpecConstDword3 = 0;
layout (constant_id = 4) const uint SpecConstDword4 = 0;
layout (constant_id = 5) const uint SpecConstDword5 = 0;
layout (constant_id = 6) const uint SpecConstDword6 = 0;
layout (constant_id = 7) const uint SpecConstDword7 = 0;
layout (constant_id = 8) const uint SpecConstDword8 = 0;
layout (constant_id = 9) const uint SpecConstDword9 = 0;
layout (constant_id = 10) const uint SpecConstDword10 = 0;
layout (constant_id = 11) const uint SpecConstDword11 = 0;
layout (constant_id = 12) const uint SpecConstDword12 = 0;

bool specIsOptimized() {
    return SpecConstDword12 != 0;
}

uint specSamplerType(uint textureStage) {
    uint dword = specIsOptimized() ? SpecConstDword0 : dynamicSpecConstDword[0];
    return bitfieldExtract(dword, int(textureStage) * 2, 2);
}
bool specSamplerIsDepth(uint textureStage) {
    uint dword = specIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 0 + int(textureStage), 1) != 0u;
}
uint specAlphaCompareOp() {
    uint dword = specIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 21, 3);
}
bool specProjected(uint textureStage) {
    uint dword = specIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 24 + int(textureStage), 1) != 0u;
}

bool specSamplerIsNull(uint textureStage) {
    uint dword = specIsOptimized() ? SpecConstDword2 : dynamicSpecConstDword[2];
    return bitfieldExtract(dword, int(textureStage), 1) != 0;
}
uint specAlphaPrecisionBits() {
    uint dword = specIsOptimized() ? SpecConstDword2 : dynamicSpecConstDword[2];
    return bitfieldExtract(dword, 21, 4);
}
bool specFogEnabled() {
    uint dword = specIsOptimized() ? SpecConstDword2 : dynamicSpecConstDword[2];
    return bitfieldExtract(dword, 25, 1) != 0;
}
uint specPixelFogMode() {
    uint dword = specIsOptimized() ? SpecConstDword2 : dynamicSpecConstDword[2];
    return bitfieldExtract(dword, 28, 2);
}

bool specFetch4(uint textureStage) {
    uint dword = specIsOptimized() ? SpecConstDword4 : dynamicSpecConstDword[4];
    return bitfieldExtract(dword, int(textureStage), 1) != 0u;
}

bool specDrefClamp(uint textureStage) {
    uint dword = specIsOptimized() ? SpecConstDword5 : dynamicSpecConstDword[5];
    return bitfieldExtract(dword, int(textureStage), 1) != 0u;
}
uint specClipPlaneCount() {
    uint dword = specIsOptimized() ? SpecConstDword5 : dynamicSpecConstDword[5];
    return bitfieldExtract(dword, 21, 3);
}
uint specPointMode() {
    uint dword = specIsOptimized() ? SpecConstDword5 : dynamicSpecConstDword[5];
    return bitfieldExtract(dword, 24, 2);
}
uint specDrefScaling() {
    uint dword = specIsOptimized() ? SpecConstDword5 : dynamicSpecConstDword[5];
    return bitfieldExtract(dword, 26, 5);
}

uint specDword(uint index) {
    switch (index) {
        case 0:
            return SpecConstDword0;
        case 1:
            return SpecConstDword1;
        case 2:
            return SpecConstDword2;
        case 3:
            return SpecConstDword3;
        case 4:
            return SpecConstDword4;
        case 5:
            return SpecConstDword5;
        case 6:
            return SpecConstDword6;
        case 7:
            return SpecConstDword7;
        case 8:
            return SpecConstDword8;
        case 9:
            return SpecConstDword9;
        case 10:
            return SpecConstDword10;
        case 11:
            return SpecConstDword11;
        case 12:
            return SpecConstDword12;
        default:
            return 0;
    }
}

uint specActiveTextureStages() {
    uint dword = specIsOptimized() ? SpecConstDword4 : dynamicSpecConstDword[4];
    return bitfieldExtract(dword, 16, 3) + 1u;
}

bool specGlobalSpecularEnabled() {
    uint dword = specIsOptimized() ? SpecConstDword6 : dynamicSpecConstDword[6];
    return bitfieldExtract(dword, 31, 1) != 0;
}

uint specTextureStageColorOp(uint textureStage) {
    uint dword = specIsOptimized() ? specDword(6 + textureStage) : dynamicSpecConstDword[6 + textureStage];
    return bitfieldExtract(dword, 0, 5);
}

uint specTextureStageColorArg(uint textureStage, uint arg) {
    uint dword = specIsOptimized() ? specDword(6 + textureStage) : dynamicSpecConstDword[6 + textureStage];
    uint value = bitfieldExtract(dword, 5 + 5 * int(arg - 1u), 5);
    // Move the flags by 1 bit. 0x18 = 0b11000
    value = (value & ~0x18) | ((value & 0x18) << 1u);
    return value;
}

uint specTextureStageAlphaOp(uint textureStage) {
    uint dword = specIsOptimized() ? specDword(6 + textureStage) : dynamicSpecConstDword[6 + textureStage];
    return bitfieldExtract(dword, 15, 5);
}

uint specTextureStageAlphaArg(uint textureStage, uint arg) {
    uint dword = specIsOptimized() ? specDword(6 + textureStage) : dynamicSpecConstDword[6 + textureStage];
    uint value = bitfieldExtract(dword, 20 + 5 * int(arg - 1u), 5);
    // Move the flags by 1 bit. 0x18 = 0b11000
    value = (value & ~0x18) | ((value & 0x18) << 1u);
    return value;
}

bool specTextureStageResultIsTemp(uint textureStage) {
    uint dword = specIsOptimized() ? specDword(6 + textureStage) : dynamicSpecConstDword[6 + textureStage];
    return bitfieldExtract(dword, 30, 1) != 0;
}


vec4 calculateFog(vec4 vPos, vec4 oColor) {
    vec3 fogColor = vec3(rs.fogColor[0], rs.fogColor[1], rs.fogColor[2]);
    float fogScale = rs.fogScale;
    float fogEnd = rs.fogEnd;
    float fogDensity = rs.fogDensity;
    D3DFOGMODE fogMode = specPixelFogMode();
    bool fogEnabled = specFogEnabled();
    if (!fogEnabled) {
        return oColor;
    }

    float w = vPos.w;
    float z = vPos.z;
    float depth = z * (1.0 / w);
    float fogFactor;
    switch (fogMode) {
        case D3DFOG_NONE:
            fogFactor = in_Fog;
            break;

        // (end - d) / (end - start)
        case D3DFOG_LINEAR:
            fogFactor = fogEnd - depth;
            fogFactor = fogFactor * fogScale;
            fogFactor = spvNClamp(fogFactor, 0.0, 1.0);
            break;

        // 1 / (e^[d * density])^2
        case D3DFOG_EXP2:
        // 1 / (e^[d * density])
        case D3DFOG_EXP:
            fogFactor = depth * fogDensity;

            if (fogMode == D3DFOG_EXP2)
                fogFactor *= fogFactor;

            // Provides the rcp.
            fogFactor = -fogFactor;
            fogFactor = exp(fogFactor);
            break;
    }

    vec4 color = oColor;
    vec3 color3 = color.rgb;
    vec3 fogFact3 = vec3(fogFactor);
    vec3 lerpedFrog = mix(fogColor, color3, fogFact3);
    return vec4(lerpedFrog.r, lerpedFrog.g, lerpedFrog.b, color.a);
}


// [D3D8] Scale Dref to [0..(2^N - 1)] for D24S8 and D16 if Dref scaling is enabled
vec4 adjustDref(vec4 texCoord, uint referenceComponentIndex, uint samplerIndex) {
    float reference = texCoord[referenceComponentIndex];
    uint drefScaleFactor = specDrefScaling();
    if (drefScaleFactor != 0) {
        float maxDref = 1.0 / (float(1 << drefScaleFactor) - 1.0);
        reference *= maxDref;
        texCoord[referenceComponentIndex] = reference;
    }
    if (specDrefClamp(samplerIndex)) {
        texCoord[referenceComponentIndex] = clamp(reference, 0.0, 1.0);
    }
    return texCoord;
}


vec4 calculateBumpmapCoords(uint stage, vec4 baseCoords, vec4 previousStageTextureVal) {
    uint previousStage = stage - 1;

    vec4 coords = baseCoords;
    [[unroll]]
    for (uint i = 0; i < 2; i++) {
        float tc_m_n = coords[i];
        vec2 bm = vec2(sharedData.Stages[previousStage].BumpEnvMat[i][0], sharedData.Stages[previousStage].BumpEnvMat[i][1]);
        vec2 t = previousStageTextureVal.xy;
        float result = tc_m_n + dot(bm, t);
        coords[i] = result;
    }
    return coords;
}


uint loadSamplerHeapIndex(uint samplerBindingIndex) {
    uint packedSamplerIndex = packedSamplerIndices[samplerBindingIndex / 2u];
    return bitfieldExtract(packedSamplerIndex, 16 * (int(samplerBindingIndex) & 1), 16);
}


vec4 sampleTexture(uint stage, vec4 texcoord, vec4 previousStageTextureVal) {
    if (specProjected(stage)) {
        texcoord /= texcoord.w;
    }

    uint previousStageColorOp = 0;
    if (stage > 0) {
        bool isPreviousStageOptimized = specIsOptimized() && stage - 1 < SpecConstOptimizedTextureStageCount;
        previousStageColorOp = isPreviousStageOptimized ? specTextureStageColorOp(stage - 1) : colorOp(stage - 1);
    }

    if (stage != 0 && (
        previousStageColorOp == D3DTOP_BUMPENVMAP
        || previousStageColorOp == D3DTOP_BUMPENVMAPLUMINANCE)) {
        texcoord = calculateBumpmapCoords(stage, texcoord, previousStageTextureVal);
    }

    vec4 texVal;
    uint textureType = D3DRTYPE_TEXTURE + specSamplerType(stage);
    switch (textureType) {
        case D3DRTYPE_TEXTURE:
            if (specSamplerIsDepth(stage))
                texVal = texture(sampler2DShadow(t2d[stage], sampler_heap[loadSamplerHeapIndex(stage)]), adjustDref(texcoord, 2, stage).xyz).xxxx;
            else
                texVal = texture(sampler2D(t2d[stage], sampler_heap[loadSamplerHeapIndex(stage)]), texcoord.xy);
            break;
        case D3DRTYPE_CUBETEXTURE:
            if (specSamplerIsDepth(stage))
                texVal = texture(samplerCubeShadow(tcube[stage], sampler_heap[loadSamplerHeapIndex(stage)]), adjustDref(texcoord, 3, stage)).xxxx;
            else
                texVal = texture(samplerCube(tcube[stage], sampler_heap[loadSamplerHeapIndex(stage)]), texcoord.xyz);
            break;
        case D3DRTYPE_VOLUMETEXTURE:
            texVal = texture(sampler3D(t3d[stage], sampler_heap[loadSamplerHeapIndex(stage)]), texcoord.xyz);
            break;
    }

    if (stage != 0 && previousStageColorOp == D3DTOP_BUMPENVMAPLUMINANCE) {
        float lScale = sharedData.Stages[stage - 1].BumpEnvLScale;
        float lOffset = sharedData.Stages[stage - 1].BumpEnvLOffset;
        float scale = texVal.z;
        scale *= lScale;
        scale += lOffset;
        scale = clamp(scale, 0.0, 1.0);
        texVal *= scale;
    }

    return texVal;
}


vec4 readArgValue(uint stage, uint arg, vec4 current, vec4 temp, vec4 textureVal) {
    vec4 reg = vec4(1.0);
    switch (arg & D3DTA_SELECTMASK) {
        case D3DTA_CONSTANT:
            reg = vec4(
                sharedData.Stages[stage].Constant[0],
                sharedData.Stages[stage].Constant[1],
                sharedData.Stages[stage].Constant[2],
                sharedData.Stages[stage].Constant[3]
            );
            break;
        case D3DTA_CURRENT:
            reg = current;
            break;
        case D3DTA_DIFFUSE:
            reg = in_Color0;
            break;
        case D3DTA_SPECULAR:
            reg = in_Color1;
            break;
        case D3DTA_TEMP:
            reg = temp;
            break;
        case D3DTA_TEXTURE:
            reg = textureVal;
            break;
        case D3DTA_TFACTOR:
            reg = data.textureFactor;
            break;
    }

    // reg = 1 - reg
    if ((arg & D3DTA_COMPLEMENT) != 0)
        reg = vec4(1.0) - reg;

    // reg = reg.wwww
    if ((arg & D3DTA_ALPHAREPLICATE) != 0)
        reg = reg.aaaa;

    return reg;
}

vec4[TextureArgCount] readArgValues(uint stage, uint args[TextureArgCount], vec4 current, vec4 temp, vec4 textureVal) {
    vec4 argVals[TextureArgCount];
    [[unroll]]
    for (uint argI = 0; argI < TextureArgCount; argI++) {
        argVals[argI] = readArgValue(stage, args[argI], current, temp, textureVal);
    }
    return argVals;
}

vec4 complement(vec4 val) {
    return vec4(1.0) - val;
}

vec4 saturate(vec4 val) {
    return clamp(val, vec4(0.0), vec4(1.0));
}

vec4 calculateTextureStage(uint op, vec4 dst, vec4 arg[TextureArgCount], vec4 current, vec4 textureVal) {
    switch (op) {
        case D3DTOP_SELECTARG1:
            return arg[1];

        case D3DTOP_SELECTARG2:
            return arg[2];

        case D3DTOP_MODULATE4X:
            return arg[1] * arg[2] * 4.0;

        case D3DTOP_MODULATE2X:
            return arg[1] * arg[2] * 2.0;

        case D3DTOP_MODULATE:
            return arg[1] * arg[2];

        case D3DTOP_ADDSIGNED2X:
            return saturate(2.0 * (arg[1] + (arg[2] - vec4(0.5))));

        case D3DTOP_ADDSIGNED:
            return saturate(arg[1] + (arg[2] - vec4(0.5)));

        case D3DTOP_ADD:
            return saturate(arg[1] + arg[2]);

        case D3DTOP_SUBTRACT:
            return saturate(arg[1] - arg[2]);

        case D3DTOP_ADDSMOOTH:
            return fma(complement(arg[1]), arg[2], arg[1]);

        case D3DTOP_BLENDDIFFUSEALPHA:
            return mix(arg[2], arg[1], in_Color0.aaaa);

        case D3DTOP_BLENDTEXTUREALPHA:
            return mix(arg[2], arg[1], textureVal.aaaa);

        case D3DTOP_BLENDFACTORALPHA:
            return mix(arg[2], arg[1], data.textureFactor.aaaa);

        case D3DTOP_BLENDTEXTUREALPHAPM:
            return saturate(fma(arg[2], complement(textureVal.aaaa), arg[1]));

        case D3DTOP_BLENDCURRENTALPHA:
            return mix(arg[2], arg[1], current.aaaa);

        case D3DTOP_PREMODULATE:
            return dst; // Not implemented

        case D3DTOP_MODULATEALPHA_ADDCOLOR:
            return saturate(fma(arg[1].aaaa, arg[2], arg[1]));

        case D3DTOP_MODULATECOLOR_ADDALPHA:
            return saturate(fma(arg[1], arg[2], arg[1].aaaa));

        case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
            return saturate(fma(complement(arg[1].aaaa), arg[2], arg[1]));

        case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
            return saturate(fma(complement(arg[1]), arg[2], arg[1].aaaa));

        case D3DTOP_BUMPENVMAPLUMINANCE:
        case D3DTOP_BUMPENVMAP:
            // Load texture for the next stage...
            return dst;

        case D3DTOP_DOTPRODUCT3:
            return saturate(vec4(dot(arg[1].rgb - vec3(0.5), arg[2].rgb - vec3(0.5)) * 4.0));

        case D3DTOP_MULTIPLYADD:
            return saturate(fma(arg[1], arg[2], arg[0]));

        case D3DTOP_LERP:
            return mix(arg[2], arg[1], arg[0]);

        default:
            // Unhandled texture op!
            return dst;

    }

    return vec4(0.0);
}


void alphaTest() {
    uint alphaFunc = specAlphaCompareOp();
    uint alphaPrecision = specAlphaPrecisionBits();
    uint alphaRefInitial = rs.alphaRef;
    float alphaRef;
    float alpha = out_Color0.a;

    if (alphaFunc == VK_COMPARE_OP_ALWAYS) {
        return;
    }

    // Check if the given bit precision is supported
    bool useIntPrecision = alphaPrecision <= 8;
    if (useIntPrecision) {
        // Adjust alpha ref to the given range
        uint alphaRefInt = (alphaRefInitial << alphaPrecision) | (alphaRefInitial >> (8 - alphaPrecision));

        // Convert alpha ref to float since we'll do the comparison based on that
        alphaRef = float(alphaRefInt);

        // Adjust alpha to the given range and round
        float alphaFactor = float((256u << alphaPrecision) - 1u);

        alpha = round(alpha * alphaFactor);
    } else {
        alphaRef = float(alphaRefInitial) / 255.0;
    }

    bool atestResult;
    switch (alphaFunc) {
        case VK_COMPARE_OP_NEVER:
            atestResult = false;
            break;

        case VK_COMPARE_OP_LESS:
            atestResult = alpha < alphaRef;
            break;

        case VK_COMPARE_OP_EQUAL:
            atestResult = alpha == alphaRef;
            break;

        case VK_COMPARE_OP_LESS_OR_EQUAL:
            atestResult = alpha <= alphaRef;
            break;

        case VK_COMPARE_OP_GREATER:
            atestResult = alpha > alphaRef;
            break;

        case VK_COMPARE_OP_NOT_EQUAL:
            atestResult = alpha != alphaRef;
            break;

        case VK_COMPARE_OP_GREATER_OR_EQUAL:
            atestResult = alpha >= alphaRef;
            break;

        default:
        case VK_COMPARE_OP_ALWAYS:
            atestResult = true;
            break;
    }

    bool atestDiscard = !atestResult;
    if (atestDiscard) {
        discard;
    }
}

struct TextureStageState {
    vec4 current;
    vec4 temp;
    vec4 previousStageTextureVal;
};

TextureStageState runTextureStage(uint stage, TextureStageState state) {
    if (specActiveTextureStages() <= stage) {
        return state;
    }

    const bool isStageOptimized = specIsOptimized() && stage < SpecConstOptimizedTextureStageCount;

    const uint colorOp = isStageOptimized ? specTextureStageColorOp(stage) : colorOp(stage);

    // This cancels all subsequent stages.
    if (colorOp == D3DTOP_DISABLE)
        return state;

    const bool resultIsTemp = isStageOptimized ? specTextureStageResultIsTemp(stage) : resultIsTemp(stage);
    vec4 dst = resultIsTemp ? state.temp : state.current;

    const uint alphaOp = isStageOptimized ? specTextureStageAlphaOp(stage) : alphaOp(stage);

    const bool usesArg0 = !isStageOptimized
        || colorOp == D3DTOP_LERP
        || colorOp == D3DTOP_MULTIPLYADD
        || alphaOp == D3DTOP_LERP
        || alphaOp == D3DTOP_MULTIPLYADD;

    const uint colorArgs[TextureArgCount] = {
        usesArg0 ? colorArg0(stage) : D3DTA_CONSTANT,
        isStageOptimized ? specTextureStageColorArg(stage, 1u) : colorArg1(stage),
        isStageOptimized ? specTextureStageColorArg(stage, 2u) : colorArg2(stage)
    };
    const uint alphaArgs[TextureArgCount] = {
        usesArg0 ? alphaArg0(stage) : D3DTA_CONSTANT,
        isStageOptimized ? specTextureStageAlphaArg(stage, 1u) : alphaArg1(stage),
        isStageOptimized ? specTextureStageAlphaArg(stage, 2u) : alphaArg2(stage)
    };

    vec4 textureVal = vec4(0.0);
    bool usesTexture = false;
    [[unroll]]
    for (uint argI = 0; argI < TextureArgCount; argI++) {
        usesTexture = usesTexture || colorArgs[argI] == D3DTA_TEXTURE || alphaArgs[argI] == D3DTA_TEXTURE;
    }
    if (usesTexture) {
        // We need to replace TEXCOORD inputs with gl_PointCoord
        // if D3DRS_POINTSPRITEENABLE is set.
        const uint pointMode = specPointMode();
        const bool isSprite = bitfieldExtract(pointMode, 1, 1) == 1u;

        vec4 texCoord;
        if (isSprite) {
            texCoord = vec4(gl_PointCoord, 0.0, 0.0);
        } else {
            switch (stage) {
                case 0: texCoord = in_Texcoord0; break;
                case 1: texCoord = in_Texcoord1; break;
                case 2: texCoord = in_Texcoord2; break;
                case 3: texCoord = in_Texcoord3; break;
                case 4: texCoord = in_Texcoord4; break;
                case 5: texCoord = in_Texcoord5; break;
                case 6: texCoord = in_Texcoord6; break;
                case 7: texCoord = in_Texcoord7; break;
            }
        }
        const vec4 unboundTextureConst = vec4(0.0, 0.0, 0.0, 1.0);
        textureVal = !specSamplerIsNull(stage) ? sampleTexture(stage, texCoord, state.previousStageTextureVal) : unboundTextureConst;
    }

    // Fast path if alpha/color path is identical.
    // D3DTOP_DOTPRODUCT3 also has special quirky behaviour here.
    const bool fastPath = colorOp == alphaOp && colorArgs == alphaArgs;
    if (fastPath || colorOp == D3DTOP_DOTPRODUCT3) {
        vec4 colorArgVals[TextureArgCount] = readArgValues(stage, colorArgs, state.current, state.temp, textureVal);
        dst = calculateTextureStage(colorOp, dst, colorArgVals, state.current, textureVal);
    } else {
        vec4 colorResult = dst;
        vec4 alphaResult = dst;

        vec4 colorArgVals[TextureArgCount] = readArgValues(stage, colorArgs, state.current, state.temp, textureVal);
        colorResult = calculateTextureStage(colorOp, dst, colorArgVals, state.current, textureVal);

        if (alphaOp != D3DTOP_DISABLE) {
            vec4 alphaArgVals[TextureArgCount] = readArgValues(stage, alphaArgs, state.current, state.temp, textureVal);
            alphaResult = calculateTextureStage(alphaOp, dst, alphaArgVals, state.current, textureVal);
        }

        dst.xyz = colorResult.xyz;

        // src0.x, src0.y, src0.z src1.w
        if (alphaOp != D3DTOP_DISABLE) {
            dst.a = alphaResult.a;
        }
    }

    if (resultIsTemp) {
        state.temp = dst;
    } else {
        state.current = dst;
    }
    state.previousStageTextureVal = textureVal;

    return state;
}

void main() {
    // in_Color0 is diffuse
    // in_Color1 is specular

    TextureStageState state;
    // Current starts of as equal to diffuse.
    state.current = in_Color0;
    // Temp starts off as equal to vec4(0)
    state.temp = vec4(0.0);
    state.previousStageTextureVal = vec4(0.0);

    // If we turn this into a loop, performance becomes very poor on the proprietary Nvidia driver
    // because it fails to unroll it.
    state = runTextureStage(0, state);
    state = runTextureStage(1, state);
    state = runTextureStage(2, state);
    state = runTextureStage(3, state);
    state = runTextureStage(4, state);
    state = runTextureStage(5, state);
    state = runTextureStage(6, state);
    state = runTextureStage(7, state);

    if (globalSpecularEnabled()) {
        vec4 specular = in_Color1 * vec4(1.0, 1.0, 1.0, 0.0);
        state.current += specular;
    }

    state.current = calculateFog(gl_FragCoord, state.current);

    out_Color0 = state.current;

    alphaTest();
}
