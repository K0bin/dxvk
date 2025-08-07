#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_spirv_intrinsics : require
#extension GL_EXT_demote_to_helper_invocation : require
#extension GL_ARB_derivative_control : require

#extension GL_EXT_nonuniform_qualifier : require // TODO: Get rid of this

#define GLSL
#include "../d3d9_shader_types.h"

/*1
#define FLOAT_MAX_VALUE 340282346638528859811704183484516925440.0


const uint MaxClipPlaneCount = 6;
*/
const uint TextureStageCount = 8;
const uint TextureArgCount = 3;


layout(constant_id = 0) const int drefScaling = 0;

// The locations need to match with RegisterLinkerSlot in dxso_util.cpp
layout(location = 0) in vec4 in_Normal;
layout(location = 1) in vec4 in_Texcoords[8]; // TODO: Is the array compatible with separate out values on the other side
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
};


// Dynamic "spec constants"
// See d3d9_spec_constants.h for packing
// MaxSpecDwords = 6
// Binding has to match with getSpecConstantBufferSlot in dxso_util.h
layout(set = 0, binding = 31, scalar) uniform SpecConsts {
    uint specConstDword[6];
};


// TODO: Are the arrays compatible with how DXVK binds textures
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
// See D3D9FFShaderStage in d3d9_shader_types.h
// Please, dearest compiler, inline all of this.
uint ColorOp(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 0, 5);
}
uint ColorArg0(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 5, 6);
}
uint ColorArg1(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 11, 6);
}
uint ColorArg2(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[0], 17, 6);
}

uint AlphaOp(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 0, 5);
}
uint AlphaArg0(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 5, 6);
}
uint AlphaArg1(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 11, 6);
}
uint AlphaArg2(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[1], 17, 6);
}

uint TextureType(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[2], 0, 2);
}
bool ResultIsTemp(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[2], 2, 1) != 0;
}
bool Projected(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[2], 3, 1) != 0;
}
uint ProjectedCount(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[2], 4, 3);
}
bool SampleDref(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[2], 7, 1) != 0;
}
bool TextureBound(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[2], 8, 1) != 0;
}
bool GlobalSpecularEnable(uint stageIndex) {
    return bitfieldExtract(data.Stages[stageIndex].Primitive[2], 9, 1) != 0;
}


// Functions to extract information from the packed dynamic spec consts
// See d3d9_spec_constants.h for packing
// Please, dearest compiler, inline all of this.
uint SpecSamplerType() {
    return bitfieldExtract(specConstDword[0], 0, 32);
}
uint SpecSamplerDepthMode() {
    return bitfieldExtract(specConstDword[1], 0, 21);
}
uint SpecAlphaCompareOp() {
    return bitfieldExtract(specConstDword[1], 21, 3);
}
uint SpecPointMode() {
    return bitfieldExtract(specConstDword[1], 24, 2);
}
uint SpecVertexFogMode() {
    return bitfieldExtract(specConstDword[1], 26, 2);
}
uint SpecPixelFogMode() {
    return bitfieldExtract(specConstDword[1], 28, 2);
}
bool SpecFogEnabled() {
    return bitfieldExtract(specConstDword[1], 30, 1) != 0;
}
uint SpecSamplerNull() {
    return bitfieldExtract(specConstDword[2], 0, 21);
}
uint SpecProjectionType() {
    return bitfieldExtract(specConstDword[2], 21, 6);
}
uint SpecAlphaPrecisionBits() {
    return bitfieldExtract(specConstDword[2], 27, 4);
}
uint SpecVertexShaderBools() {
    return bitfieldExtract(specConstDword[3], 0, 16);
}
uint SpecPixelShaderBools() {
    return bitfieldExtract(specConstDword[3], 16, 16);
}
uint SpecFetch4() {
    return bitfieldExtract(specConstDword[4], 0, 16);
}
uint SpecDrefClamp() {
    return bitfieldExtract(specConstDword[5], 0, 21);
}
uint SpecClipPlaneCount() {
    return bitfieldExtract(specConstDword[5], 21, 3);
}


/*#define isPixel true
vec4 DoFixedFunctionFog(vec4 vPos, vec4 oColor) {
    vec4 color1 = HasColor1() ? in_Color1 : vec4(0.0);

    vec4 specular = !isPixel ? color1 : vec4(0.0);
    bool hasSpecular = !isPixel && HasColor1();

    vec3 fogColor = vec3(rs.fogColor[0], rs.fogColor[1], rs.fogColor[2]);
    float fogScale = rs.fogScale;
    float fogEnd = rs.fogEnd;
    float fogDensity = rs.fogDensity;
    D3DFOGMODE fogMode = isPixel ? SpecPixelFogMode() : SpecVertexFogMode();
    bool fogEnabled = SpecFogEnabled();
    if (!fogEnabled)
        return vec4(0.0);

    float w = vPos.w;
    float z = vPos.z;
    float depth;
    if (isPixel) {
        depth = z * (1.0 / w);
    } else {
        if (RangeFog()) {
            vec3 pos3 = vPos.xyz;
            depth = length(pos3);
        } else {
            depth = HasFog() ? in_Fog : abs(z);
        }
    }
    float fogFactor;
    if (!isPixel && HasPositionT()) {
        fogFactor = hasSpecular ? specular.w : 1.0;
    } else {
        switch (fogMode) {
            case D3DFOG_NONE:
                if (isPixel)
                    fogFactor = in_Fog;
                else if (hasSpecular)
                    fogFactor = specular.w;
                else
                    fogFactor = 1.0;
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
    }

    if (isPixel) {
        vec4 color = oColor;
        vec3 color3 = color.xyz;
        vec3 fogFact3 = vec3(fogFactor);
        vec3 lerpedFrog = mix(color3, fogColor, fogFact3);
        return vec4(lerpedFrog.x, lerpedFrog.y, lerpedFrog.z, color.z);
    } else {
        return vec4(fogFactor);
    }
}


float DoPointSize(vec4 vtx) {
    float value = in_PointSize != 0.0 ? in_PointSize : rs.pointSize;
    uint pointMode = SpecPointMode();
    bool isScale = bitfieldExtract(pointMode, 0, 1) != 0;
    float scaleC = rs.pointScaleC;
    float scaleB = rs.pointScaleB;
    float scaleA = rs.pointScaleA;

    vec3 vtx3 = vtx.xyz;

    float DeSqr = dot(vtx3, vtx3);
    float De    = sqrt(DeSqr);
    float scaleValue = scaleC * DeSqr;
          scaleValue = fma(scaleB, De, scaleValue);
          scaleValue += scaleA;
          scaleValue = sqrt(scaleValue);
          scaleValue = value / scaleValue;

    value = isScale ? scaleValue : value;

    float pointSizeMin = rs.pointSizeMin;
    float pointSizeMax = rs.pointSizeMax;

    return clamp(value, pointSizeMin, pointSizeMax);
}


void emitVsClipping(vec4 vtx) {
    vec4 worldPos = data.InverseView * vtx;

    // Always consider clip planes enabled when doing GPL by forcing 6 for the quick value.
    uint clipPlaneCount = SpecClipPlaneCount();

    // Compute clip distances
    for (uint i = 0; i < MaxClipPlaneCount; i++) {
        vec4 clipPlane = clipPlanes[i];
        float dist = dot(worldPos, clipPlane);
        bool clipPlaneEnabled = i < clipPlaneCount;
        float value = clipPlaneEnabled ? dist : 0.0;
        gl_ClipDistance[i] = value;
    }
}


vec4 PickSource(uint Source, vec4 Material) {
    if (Source == D3DMCS_MATERIAL)
        return Material;
    else if (Source == D3DMCS_COLOR1)
        return HasColor0() ? in_Color0 : vec4(0.0);
    else
        return HasColor1() ? in_Color1 : vec4(0.0);
}*/

// [D3D8] Scale Dref to [0..(2^N - 1)] for D24S8 and D16 if Dref scaling is enabled
vec4 scaleDref(vec4 texCoord, int referenceIdx) {
    float reference = texCoord[referenceIdx];
    if (drefScaling == 0) {
        return texCoord;
    }
    float maxDref = 1.0 / (float(1 << drefScaling) - 1.0);
    reference *= maxDref;
    texCoord[referenceIdx] = reference;
    return texCoord;
}


vec4 DoBumpmapCoords(uint stage, vec4 baseCoords) {
    stage = stage - 1;

    vec4 coords = baseCoords;
    for (uint i = 0; i < 2; i++) {
        float tc_m_n = coords[i];
        vec2 bm = vec2(sharedData.Stages[stage].BumpEnvMat[0][0], sharedData.Stages[stage].BumpEnvMat[0][1]);
        //vec2 t =
    }
    return coords;
}


// TODO: Passing the index here makes non-uniform necessary, solve that
vec4 GetTexture(uint stage) {
    uint textureType = D3DRTYPE_TEXTURE + TextureType(stage);

    vec4 texcoord = in_Texcoords[stage];

    bool shouldProject = Projected(stage);
    float projValue = 1.0;
    if (shouldProject) {
        // Always use w, the vertex shader puts the correct value there.
        projValue = texcoord.w;
        if (textureType == D3DRTYPE_TEXTURE) {
            // For 2D textures we divide by the z component, so move the w component up by one.
            texcoord.z = projValue;
        }
    }

    if (stage != 0 && (
        ColorOp(stage - 1) == D3DTOP_BUMPENVMAP
        || ColorOp(stage - 1) == D3DTOP_BUMPENVMAPLUMINANCE)) {
        if (shouldProject) {
            float projRcp = 1.0 / projValue;
            texcoord *= projRcp;
        }

        texcoord = DoBumpmapCoords(stage, texcoord);

        shouldProject = false;
    }

    vec4 texVal;
    switch (textureType) {
        case D3DRTYPE_TEXTURE:
            if (SampleDref(stage))
                texVal = texture(sampler2DShadow(t2d[stage], sampler_heap[stage]), scaleDref(texcoord, 2).xyz).xxxx;
            else if (shouldProject)
                texVal = textureProj(sampler2D(t2d[stage], sampler_heap[stage]), texcoord.xyz);
            else
                texVal = texture(sampler2D(t2d[stage], sampler_heap[stage]), texcoord.xy);
            break;
        case D3DRTYPE_CUBETEXTURE:
            if (SampleDref(stage))
                texVal = texture(samplerCubeShadow(tcube[stage], sampler_heap[stage]), scaleDref(texcoord, 3)).xxxx;
            else if (shouldProject) {
                texVal = texture(samplerCube(tcube[stage], sampler_heap[stage]), texcoord.xyz / texcoord.w); // TODO: ?
                //texVal = textureProj(samplerCube(tcube[stage], sampler_heap[stage]), texcoord.xyzw);
            } else
                texVal = texture(samplerCube(tcube[stage], sampler_heap[stage]), texcoord.xyz);
            break;
        case D3DRTYPE_VOLUMETEXTURE:
            if (SampleDref(stage))
                texVal = vec4(0.0); // TODO: ?
            else if (shouldProject)
                texVal = textureProj(sampler3D(t3d[stage], sampler_heap[stage]), texcoord);
            else
                texVal = texture(sampler3D(t3d[stage], sampler_heap[stage]), texcoord.xyz);
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


vec4 GetArg(uint stage, uint arg, vec4 current, vec4 diffuse, vec4 specular, vec4 temp) {
    vec4 reg = vec4(1.0);
    switch (arg & D3DTA_SELECTMASK) {
        case D3DTA_CONSTANT: {
            return vec4(
                sharedData.Stages[stage].Constant[0],
                sharedData.Stages[stage].Constant[1],
                sharedData.Stages[stage].Constant[2],
                sharedData.Stages[stage].Constant[3]
            );
        }
        case D3DTA_CURRENT:
            return current;
        case D3DTA_DIFFUSE:
            return diffuse;
        case D3DTA_SPECULAR:
            return specular;
        case D3DTA_TEMP:
            return temp;
        case D3DTA_TEXTURE:
            return vec4(0.0); // TODO
    }
}


vec4 DoOp(uint op, vec4 arg[TextureArgCount]) {
    switch (op) {
        case D3DTOP_SELECTARG1:
            return arg[1];

        case D3DTOP_SELECTARG2:
            return arg[2];

        case D3DTOP_MODULATE4X:
            return arg[1] * arg[2];
    }

    return vec4(0.0);
}


void main() {
    vec4 diffuse = in_Color0;
    vec4 specular = in_Color1;

    // Current starts of as equal to diffuse.
    vec4 current = diffuse;
    // Temp starts off as equal to vec4(0)
    vec4 temp = vec4(0.0);

    vec4 textureVar = vec4(0.0, 0.0, 0.0, 1.0);

    vec4 unboundTextureConst = vec4(0.0, 0.0, 0.0, 1.0);

    for (uint i = 0; i < TextureStageCount; i++) {
        uint colorOp = ColorOp(i);
        uint colorArgs[TextureArgCount] = {
            ColorArg0(i),
            ColorArg1(i),
            ColorArg2(i)
        };
        uint alphaOp = AlphaOp(i);
        uint alphaArgs[TextureArgCount] = {
            AlphaArg0(i),
            AlphaArg1(i),
            AlphaArg2(i)
        };

        // This cancels all subsequent stages.
        if (colorOp == D3DTOP_DISABLE)
            break;

        // Fast path if alpha/color path is identical.
        // D3DTOP_DOTPRODUCT3 also has special quirky behaviour here.
        bool fastPath = colorOp == alphaOp && colorArgs == alphaArgs;
        if (fastPath || colorOp == D3DTOP_DOTPRODUCT3) {

        } else {

        }
    }

}
