#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_spirv_intrinsics : require
#extension GL_EXT_demote_to_helper_invocation : require
#extension GL_ARB_derivative_control : require
#extension GL_EXT_control_flow_attributes : require

#extension GL_EXT_nonuniform_qualifier : require // TODO: Get rid of this?

#define GLSL
#include "../d3d9_shader_types.h"

const uint TextureStageCount = 8;
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
// MaxSpecDwords = 6
// Binding has to match with getSpecConstantBufferSlot in dxso_util.h
layout(set = 0, binding = 31, scalar) uniform SpecConsts {
    uint dynamicSpecConstDword[23];
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
layout (constant_id = 13) const uint SpecConstDword13 = 0;
layout (constant_id = 14) const uint SpecConstDword14 = 0;
layout (constant_id = 15) const uint SpecConstDword15 = 0;
layout (constant_id = 16) const uint SpecConstDword16 = 0;
layout (constant_id = 17) const uint SpecConstDword17 = 0;
layout (constant_id = 18) const uint SpecConstDword18 = 0;
layout (constant_id = 19) const uint SpecConstDword19 = 0;
layout (constant_id = 20) const uint SpecConstDword20 = 0;
layout (constant_id = 21) const uint SpecConstDword21 = 0;
layout (constant_id = 22) const uint SpecConstDword22 = 0;
layout (constant_id = 23) const uint SpecConstDword23 = 0;


const uint SpecIdSamplerType = 0;
const uint SpecIdSamplerIsDepth = 1;
const uint SpecIdAlphaCompareOp = 2;
const uint SpecIdProjected = 3;
const uint SpecIdSamplerIsNull = 4;
const uint SpecIdAlphaPrecisionBits = 5;
const uint SpecIdFogEnabled = 6;
const uint SpecIdVertexFogMode = 7;
const uint SpecIdPixelFogMode = 8;
const uint SpecIdVertexShaderBools = 9;
const uint SpecIdPixelShaderBools = 10;
const uint SpecIdFetch4 = 11;
const uint SpecIdDrefClamp = 12;
const uint SpecIdDrefScaling = 13;
const uint SpecIdClipPlaneCount = 14;
const uint SpecIdPointMode = 15;

const uint SpecIdFFTexcoordIndices = 16;
const uint SpecIdFFVertexHasPositionT = 17;
const uint SpecIdFFVertexHasColor0 = 18;
const uint SpecIdFFVertexHasColor1 = 19;
const uint SpecIdFFVertexHasPointSize = 20;
const uint SpecIdFFUseLighting = 21;
const uint SpecIdFFNormalizeNormals = 22;
const uint SpecIdFFLocalViewer = 23;
const uint SpecIdFFRangeFog = 24;
const uint SpecIdFFTexcoordFlags = 25;
const uint SpecIdFFDiffuseSource = 26;
const uint SpecIdFFAmbientSource = 27;
const uint SpecIdFFSpecularSource = 28;
const uint SpecIdFFEmissiveSource = 29;
const uint SpecIdFFTransformFlags = 30;
const uint SpecIdFFVertexBlendMode = 31;
const uint SpecIdFFBlendVertexIndexed = 32;
const uint SpecIdFFVertexBlendCount = 33;
const uint SpecIdFFVertexClipping = 34;

const uint SpecIdFFTextureStage0ColorOp = 35;
const uint SpecIdFFTextureStage0ColorArg0 = 36;
const uint SpecIdFFTextureStage0ColorArg1 = 37;
const uint SpecIdFFTextureStage0ColorArg2 = 38;
const uint SpecIdFFTextureStage0AlphaOp = 39;
const uint SpecIdFFTextureStage0AlphaArg0 = 40;
const uint SpecIdFFTextureStage0AlphaArg1 = 41;
const uint SpecIdFFTextureStage0AlphaArg2 = 42;
const uint SpecIdFFTextureStage0ResultIsTemp = 43;
const uint SpecIdFFTextureStage1ColorOp = 44;
const uint SpecIdFFTextureStage1ColorArg0 = 45;
const uint SpecIdFFTextureStage1ColorArg1 = 46;
const uint SpecIdFFTextureStage1ColorArg2 = 47;
const uint SpecIdFFTextureStage1AlphaOp = 48;
const uint SpecIdFFTextureStage1AlphaArg0 = 49;
const uint SpecIdFFTextureStage1AlphaArg1 = 50;
const uint SpecIdFFTextureStage1AlphaArg2 = 51;
const uint SpecIdFFTextureStage1ResultIsTemp = 52;
const uint SpecIdFFTextureStage2ColorOp = 53;
const uint SpecIdFFTextureStage2ColorArg0 = 54;
const uint SpecIdFFTextureStage2ColorArg1 = 55;
const uint SpecIdFFTextureStage2ColorArg2 = 56;
const uint SpecIdFFTextureStage2AlphaOp = 57;
const uint SpecIdFFTextureStage2AlphaArg0 = 58;
const uint SpecIdFFTextureStage2AlphaArg1 = 59;
const uint SpecIdFFTextureStage2AlphaArg2 = 60;
const uint SpecIdFFTextureStage2ResultIsTemp = 61;
const uint SpecIdFFTextureStage3ColorOp = 62;
const uint SpecIdFFTextureStage3ColorArg0 = 63;
const uint SpecIdFFTextureStage3ColorArg1 = 64;
const uint SpecIdFFTextureStage3ColorArg2 = 65;
const uint SpecIdFFTextureStage3AlphaOp = 66;
const uint SpecIdFFTextureStage3AlphaArg0 = 67;
const uint SpecIdFFTextureStage3AlphaArg1 = 68;
const uint SpecIdFFTextureStage3AlphaArg2 = 69;
const uint SpecIdFFTextureStage3ResultIsTemp = 70;
const uint SpecIdFFTextureStage4ColorOp = 71;
const uint SpecIdFFTextureStage4ColorArg0 = 72;
const uint SpecIdFFTextureStage4ColorArg1 = 73;
const uint SpecIdFFTextureStage4ColorArg2 = 74;
const uint SpecIdFFTextureStage4AlphaOp = 75;
const uint SpecIdFFTextureStage4AlphaArg0 = 76;
const uint SpecIdFFTextureStage4AlphaArg1 = 77;
const uint SpecIdFFTextureStage4AlphaArg2 = 78;
const uint SpecIdFFTextureStage4ResultIsTemp = 79;
const uint SpecIdFFTextureStage5ColorOp = 80;
const uint SpecIdFFTextureStage5ColorArg0 = 81;
const uint SpecIdFFTextureStage5ColorArg1 = 82;
const uint SpecIdFFTextureStage5ColorArg2 = 83;
const uint SpecIdFFTextureStage5AlphaOp = 84;
const uint SpecIdFFTextureStage5AlphaArg0 = 85;
const uint SpecIdFFTextureStage5AlphaArg1 = 86;
const uint SpecIdFFTextureStage5AlphaArg2 = 87;
const uint SpecIdFFTextureStage5ResultIsTemp = 88;
const uint SpecIdFFTextureStage6ColorOp = 89;
const uint SpecIdFFTextureStage6ColorArg0 = 90;
const uint SpecIdFFTextureStage6ColorArg1 = 91;
const uint SpecIdFFTextureStage6ColorArg2 = 92;
const uint SpecIdFFTextureStage6AlphaOp = 93;
const uint SpecIdFFTextureStage6AlphaArg0 = 94;
const uint SpecIdFFTextureStage6AlphaArg1 = 95;
const uint SpecIdFFTextureStage6AlphaArg2 = 96;
const uint SpecIdFFTextureStage6ResultIsTemp = 97;
const uint SpecIdFFTextureStage7ColorOp = 98;
const uint SpecIdFFTextureStage7ColorArg0 = 99;
const uint SpecIdFFTextureStage7ColorArg1 = 100;
const uint SpecIdFFTextureStage7ColorArg2 = 101;
const uint SpecIdFFTextureStage7AlphaOp = 102;
const uint SpecIdFFTextureStage7AlphaArg0 = 103;
const uint SpecIdFFTextureStage7AlphaArg1 = 104;
const uint SpecIdFFTextureStage7AlphaArg2 = 105;
const uint SpecIdFFTextureStage7ResultIsTemp = 106;
const uint SpecIdFFGlobalSpecularEnable = 107;


struct BitfieldPosition {
    uint dwordOffset;
    uint bitOffset;
    uint sizeInBits;
};

const BitfieldPosition specConstLayout[] = {
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
};

uint GetSpecConstDword(uint index) {
    switch (index) {
        default:
        case 0: return SpecConstDword0;
        case 1: return SpecConstDword1;
        case 2: return SpecConstDword2;
        case 3: return SpecConstDword3;
        case 4: return SpecConstDword4;
        case 5: return SpecConstDword5;
        case 6: return SpecConstDword6;
        case 7: return SpecConstDword7;
        case 8: return SpecConstDword8;
        case 9: return SpecConstDword9;
        case 10: return SpecConstDword10;
        case 11: return SpecConstDword11;
        case 12: return SpecConstDword12;
        case 13: return SpecConstDword13;
        case 14: return SpecConstDword14;
        case 15: return SpecConstDword15;
        case 16: return SpecConstDword16;
        case 17: return SpecConstDword17;
        case 18: return SpecConstDword18;
        case 19: return SpecConstDword19;
        case 20: return SpecConstDword20;
        case 21: return SpecConstDword21;
        case 22: return SpecConstDword22;
    }
}

bool SpecIsOptimized() {
    return SpecConstDword23 != 0;
}

uint GetSpecConst(uint specConstIndex) {
    BitfieldPosition layoutEntry = specConstLayout[specConstIndex];
    uint dword = SpecIsOptimized() ? GetSpecConstDword(layoutEntry.dwordOffset) : dynamicSpecConstDword[layoutEntry.dwordOffset];
    return bitfieldExtract(dword, int(layoutEntry.bitOffset), int(layoutEntry.sizeInBits));
}

uint SpecSamplerType(uint textureStage) {
    uint dword = SpecIsOptimized() ? SpecConstDword0 : dynamicSpecConstDword[0];
    return bitfieldExtract(dword, int(textureStage) * 2, 2);
}
bool SpecSamplerIsDepth(uint textureStage) {
    uint dword = SpecIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 0 + int(textureStage), 1) != 0u;
}
uint SpecAlphaCompareOp() {
    uint dword = SpecIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 21, 3);
}
bool SpecProjected(uint textureStage) {
    uint dword = SpecIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 24 + int(textureStage), 1) != 0u;
}

bool SpecSamplerIsNull(uint textureStage) {
    uint dword = SpecIsOptimized() ? SpecConstDword2 : dynamicSpecConstDword[2];
    return bitfieldExtract(dword, int(textureStage), 1) != 0;
}
uint SpecAlphaPrecisionBits() {
    uint dword = SpecIsOptimized() ? SpecConstDword2 : dynamicSpecConstDword[2];
    return bitfieldExtract(dword, 21, 4);
}
bool SpecFogEnabled() {
    uint dword = SpecIsOptimized() ? SpecConstDword2 : dynamicSpecConstDword[2];
    return bitfieldExtract(dword, 25, 1) != 0;
}
uint SpecPixelFogMode() {
    uint dword = SpecIsOptimized() ? SpecConstDword2 : dynamicSpecConstDword[2];
    return bitfieldExtract(dword, 28, 2);
}

bool SpecFetch4(uint textureStage) {
    uint dword = SpecIsOptimized() ? SpecConstDword4 : dynamicSpecConstDword[4];
    return bitfieldExtract(dword, int(textureStage), 1) != 0u;
}

bool SpecDrefClamp(uint textureStage) {
    uint dword = SpecIsOptimized() ? SpecConstDword5 : dynamicSpecConstDword[5];
    return bitfieldExtract(dword, int(textureStage), 1) != 0u;
}
uint SpecDrefScaling() {
    uint dword = SpecIsOptimized() ? SpecConstDword5 : dynamicSpecConstDword[5];
    return bitfieldExtract(dword, 21, 5);
}
uint SpecClipPlaneCount() {
    uint dword = SpecIsOptimized() ? SpecConstDword5 : dynamicSpecConstDword[5];
    return bitfieldExtract(dword, 26, 3);
}
uint SpecPointMode() {
    uint dword = SpecIsOptimized() ? SpecConstDword5 : dynamicSpecConstDword[5];
    return bitfieldExtract(dword, 29, 2);
}




// Functions to extract information from the packed texture stages
// See D3D9FFTextureStage in d3d9_shader_types.h
// Please, dearest compiler, inline all of this.
uint ColorOp(uint stageIndex) {
    uint enumStageDiff = SpecIdFFTextureStage1ColorOp - SpecIdFFTextureStage0ColorOp;
    return GetSpecConst(SpecIdFFTextureStage0ColorOp + enumStageDiff * stageIndex);
}
uint ColorArg(uint stageIndex, uint argIndex) {
    uint enumStageDiff = SpecIdFFTextureStage1ColorOp - SpecIdFFTextureStage0ColorOp;
    return GetSpecConst(SpecIdFFTextureStage0ColorArg0 + argIndex + enumStageDiff * stageIndex);
}

uint AlphaOp(uint stageIndex) {
    uint enumStageDiff = SpecIdFFTextureStage1ColorOp - SpecIdFFTextureStage0ColorOp;
    return GetSpecConst(SpecIdFFTextureStage0AlphaOp + enumStageDiff * stageIndex);
}
uint AlphaArg(uint stageIndex, uint argIndex) {
    uint enumStageDiff = SpecIdFFTextureStage1ColorOp - SpecIdFFTextureStage0ColorOp;
    return GetSpecConst(SpecIdFFTextureStage0AlphaArg0 + argIndex + enumStageDiff * stageIndex);
}

bool ResultIsTemp(uint stageIndex) {
    uint enumStageDiff = SpecIdFFTextureStage1ColorOp - SpecIdFFTextureStage0ColorOp;
    return GetSpecConst(SpecIdFFTextureStage0ResultIsTemp + enumStageDiff * stageIndex) != 0;
}

bool GlobalSpecularEnable(uint stageIndex) {
    return GetSpecConst(SpecIdFFGlobalSpecularEnable) != 0;
}



/*uint ColorOp(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 0, 5);
}

uint ColorArg(uint stageIndex, uint argIndex) {
    switch(argIndex) {
        case 0:
            return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 5, 6);
            break;
        case 1:
            return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 11, 6);
        break;
        case 2:
            return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 17, 6);
        break;
    }
    return 0;
}

uint AlphaOp(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 23, 5);
}

uint AlphaArg(uint stageIndex, uint argIndex) {
    switch (argIndex) {
        case 0:
            return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 0, 6);
        break;
        case 1:
            return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 6, 6);
        break;
        case 2:
            return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 12, 6);
        break;
    }
    return 0;
}

bool ResultIsTemp(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 18, 1) != 0;
}
bool GlobalSpecularEnable(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 19, 1) != 0;
}*/











vec4 DoFixedFunctionFog(vec4 vPos, vec4 oColor) {
    vec3 fogColor = vec3(rs.fogColor[0], rs.fogColor[1], rs.fogColor[2]);
    float fogScale = rs.fogScale;
    float fogEnd = rs.fogEnd;
    float fogDensity = rs.fogDensity;
    D3DFOGMODE fogMode = SpecPixelFogMode();
    bool fogEnabled = SpecFogEnabled();
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
    uint drefScaleFactor = SpecDrefScaling();
    if (drefScaleFactor != 0) {
        float maxDref = 1.0 / (float(1 << drefScaleFactor) - 1.0);
        reference *= maxDref;
        texCoord[referenceComponentIndex] = reference;
    }
    if (SpecDrefClamp(samplerIndex)) {
        texCoord[referenceComponentIndex] = clamp(reference, 0.0, 1.0);
    }
    return texCoord;
}


vec4 DoBumpmapCoords(uint stage, vec4 baseCoords, vec4 previousStageTextureVal) {
    stage = stage - 1;

    vec4 coords = baseCoords;
    [[unroll]]
    for (uint i = 0; i < 2; i++) {
        float tc_m_n = coords[i];
        vec2 bm = vec2(sharedData.Stages[stage].BumpEnvMat[0][0], sharedData.Stages[stage].BumpEnvMat[0][1]);
        vec2 t = previousStageTextureVal.xy;
        float result = tc_m_n + dot(bm, t);
        coords[i] = result;
    }
    return coords;
}


uint LoadSamplerHeapIndex(uint samplerBindingIndex) {
    uint packedSamplerIndex = packedSamplerIndices[samplerBindingIndex / 2u];
    return bitfieldExtract(packedSamplerIndex, 16 * (int(samplerBindingIndex) & 1), 16);
}


// TODO: Passing the index here makes non-uniform necessary, solve that
vec4 GetTexture(uint stage, vec4 texcoord, vec4 previousStageTextureVal) {
    if (SpecProjected(stage)) {
        texcoord /= texcoord.w;
    }

    if (stage != 0 && (
        ColorOp(stage - 1) == D3DTOP_BUMPENVMAP
        || ColorOp(stage - 1) == D3DTOP_BUMPENVMAPLUMINANCE)) {
        texcoord = DoBumpmapCoords(stage, texcoord, previousStageTextureVal);
    }

    vec4 texVal;
    uint textureType = D3DRTYPE_TEXTURE + SpecSamplerType(stage);
    switch (textureType) {
        case D3DRTYPE_TEXTURE:
            if (SpecSamplerIsDepth(stage))
                texVal = texture(sampler2DShadow(t2d[stage], sampler_heap[LoadSamplerHeapIndex(stage)]), adjustDref(texcoord, 2, stage).xyz).xxxx;
            else
                texVal = texture(sampler2D(t2d[stage], sampler_heap[LoadSamplerHeapIndex(stage)]), texcoord.xy);
            break;
        case D3DRTYPE_CUBETEXTURE:
            if (SpecSamplerIsDepth(stage))
                texVal = texture(samplerCubeShadow(tcube[stage], sampler_heap[LoadSamplerHeapIndex(stage)]), adjustDref(texcoord, 3, stage)).xxxx;
            else
                texVal = texture(samplerCube(tcube[stage], sampler_heap[LoadSamplerHeapIndex(stage)]), texcoord.xyz);
            break;
        case D3DRTYPE_VOLUMETEXTURE:
            texVal = texture(sampler3D(t3d[stage], sampler_heap[LoadSamplerHeapIndex(stage)]), texcoord.xyz);
            break;
    }

    if (stage != 0 && ColorOp(stage - 1) == D3DTOP_BUMPENVMAPLUMINANCE) {
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


vec4 GetArg(uint stage, uint arg, vec4 current, vec4 temp, vec4 textureVal) {
    vec4 reg = vec4(1.0);
    switch (arg & D3DTA_SELECTMASK) {
        case D3DTA_CONSTANT: {
             reg = vec4(
                sharedData.Stages[stage].Constant[0],
                sharedData.Stages[stage].Constant[1],
                sharedData.Stages[stage].Constant[2],
                sharedData.Stages[stage].Constant[3]
            );
            break;
        }
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
    }

    // reg = 1 - reg
    if ((arg & D3DTA_COMPLEMENT) != 0)
        reg = vec4(1.0) - reg;

    // reg = reg.wwww
    if ((arg & D3DTA_ALPHAREPLICATE) != 0)
        reg = reg.aaaa;

    return reg;
}

vec4[TextureArgCount] ProcessArgs(uint stage, uint args[TextureArgCount], vec4 current, vec4 temp, vec4 textureVal) {
    vec4 argVals[TextureArgCount];
    [[unroll]]
    for (uint argI = 0; argI < TextureArgCount; argI++) {
        argVals[argI] = GetArg(stage, args[argI], current, temp, textureVal);
    }
    return argVals;
}

vec4 complement(vec4 val) {
    return vec4(1.0) - val;
}

vec4 saturate(vec4 val) {
    return clamp(val, vec4(0.0), vec4(1.0));
}

vec4 DoOp(uint op, vec4 dst, vec4 arg[TextureArgCount], vec4 current, vec4 textureVal) {
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


void alphaTestPS() {
    uint alphaFunc = SpecAlphaCompareOp();
    uint alphaPrecision = SpecAlphaPrecisionBits();
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

        case VK_COMPARE_OP_LESS:
            atestResult = alpha < alphaRef;

        case VK_COMPARE_OP_EQUAL:
            atestResult = alpha == alphaRef;

        case VK_COMPARE_OP_LESS_OR_EQUAL:
            atestResult = alpha <= alphaRef;

        case VK_COMPARE_OP_GREATER:
            atestResult = alpha > alphaRef;

        case VK_COMPARE_OP_NOT_EQUAL:
            atestResult = alpha != alphaRef;

        case VK_COMPARE_OP_GREATER_OR_EQUAL:
            atestResult = alpha >= alphaRef;

        default:
        case VK_COMPARE_OP_ALWAYS:
            atestResult = true;
    }

    bool atestDiscard = !atestResult;
    if (atestDiscard) {
        discard;
    }
}

void main() {
    // in_Color0 is diffuse
    // in_Color1 is specular

    // Current starts of as equal to diffuse.
    vec4 current = in_Color0;
    // Temp starts off as equal to vec4(0)
    vec4 temp = vec4(0.0);

    vec4 unboundTextureConst = vec4(0.0, 0.0, 0.0, 1.0);

    vec4 previousStageTextureVal = vec4(0.0);

    uint pointMode = SpecPointMode();
    bool isSprite = bitfieldExtract(pointMode, 1, 1) == 1u;

    [[unroll]]
    for (uint i = 0; i < TextureStageCount; i++) {
        vec4 dst = ResultIsTemp(i) ? temp : current;

        uint colorOp = ColorOp(i);

        // This cancels all subsequent stages.
        if (colorOp == D3DTOP_DISABLE)
            break;

        uint colorArgs[TextureArgCount] = {
            ColorArg(i, 0),
            ColorArg(i, 1),
            ColorArg(i, 2)
        };
        uint alphaOp = AlphaOp(i);
        uint alphaArgs[TextureArgCount] = {
            AlphaArg(i, 0),
            AlphaArg(i, 1),
            AlphaArg(i, 2)
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
            vec4 texCoord;
            if (isSprite) {
                texCoord = vec4(gl_PointCoord, 0.0, 0.0);
            } else {
                switch (i) {
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
            textureVal = !SpecSamplerIsNull(i) ? GetTexture(i, texCoord, previousStageTextureVal) : vec4(0.0, 0.0, 0.0, 1.0);
        }

        // Fast path if alpha/color path is identical.
        // D3DTOP_DOTPRODUCT3 also has special quirky behaviour here.
        bool fastPath = colorOp == alphaOp && colorArgs == alphaArgs;
        if (fastPath || colorOp == D3DTOP_DOTPRODUCT3) {
            vec4 colorArgVals[TextureArgCount] = ProcessArgs(i, colorArgs, current, temp, textureVal);
            dst = DoOp(colorOp, dst, colorArgVals, current, textureVal);
        } else {
            vec4 colorResult = dst;
            vec4 alphaResult = dst;

            vec4 colorArgVals[TextureArgCount] = ProcessArgs(i, colorArgs, current, temp, textureVal);
            colorResult = DoOp(colorOp, dst, colorArgVals, current, textureVal);

            if (alphaOp != D3DTOP_DISABLE) {
                vec4 alphaArgVals[TextureArgCount] = ProcessArgs(i, alphaArgs, current, temp, textureVal);
                alphaResult = DoOp(alphaOp, dst, alphaArgVals, current, textureVal);
            }

            // src0.x, src0.y, src0.z src1.w
            dst = vec4(colorResult.rgb, dst.a);

            // src0.x, src0.y, src0.z src1.w
            // But we flip src0, src1 to be inverse of color.
            dst = vec4(dst.rgb, alphaResult.a);
        }

        if (ResultIsTemp(i)) {
            temp = dst;
        } else {
            current = dst;
        }
        previousStageTextureVal = textureVal;
    }

    // TODO: Should this be done per-stage?
    // The FF generator only uses stage 0
    if (GlobalSpecularEnable(0)) {
        vec4 specular = in_Color1 * vec4(1.0, 1.0, 1.0, 0.0);
        current += specular;
    }

    current = DoFixedFunctionFog(gl_FragCoord, current);

    out_Color0 = current;

        if (!SpecIsOptimized()) {
            out_Color0.b = 1.0;
        }

    alphaTestPS();
}
