#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_spirv_intrinsics : require

#define GLSL
#include "../d3d9_shader_types.h"

#define FLOAT_MAX_VALUE 340282346638528859811704183484516925440.0


const uint MaxClipPlaneCount = 6;
const uint TextureStageCount = 8;


layout(location = 0) in vec4 in_Position0;
layout(location = 1) in vec4 in_Normal0;
layout(location = 2) in vec4 in_Position1;
layout(location = 3) in vec4 in_Normal1;
layout(location = 4) in vec4 in_Texcoord0;
layout(location = 5) in vec4 in_Texcoord1;
layout(location = 6) in vec4 in_Texcoord2;
layout(location = 7) in vec4 in_Texcoord3;
layout(location = 8) in vec4 in_Texcoord4;
layout(location = 9) in vec4 in_Texcoord5;
layout(location = 10) in vec4 in_Texcoord6;
layout(location = 11) in vec4 in_Texcoord7;
layout(location = 12) in vec4 in_Color0;
layout(location = 13) in vec4 in_Color1;
layout(location = 14) in float in_Fog;
layout(location = 15) in float in_PointSize;
layout(location = 16) in vec4 in_BlendWeight;
layout(location = 17) in vec4 in_BlendIndices;


// The locations need to match with RegisterLinkerSlot in dxso_util.cpp
precise gl_Position;
out float gl_ClipDistance[MaxClipPlaneCount];
layout(location = 0) out vec4 out_Normal;
layout(location = 1) out vec4 out_Texcoord0;
layout(location = 2) out vec4 out_Texcoord1;
layout(location = 3) out vec4 out_Texcoord2;
layout(location = 4) out vec4 out_Texcoord3;
layout(location = 5) out vec4 out_Texcoord4;
layout(location = 6) out vec4 out_Texcoord5;
layout(location = 7) out vec4 out_Texcoord6;
layout(location = 8) out vec4 out_Texcoord7;
layout(location = 9) out vec4 out_Color0;
layout(location = 10) out vec4 out_Color1;
layout(location = 11) out float out_Fog;


// Bindings have to match with computeResourceSlotId in dxso_util.h
// computeResourceSlotId(
//     DxsoProgramType::VertexShader,
//     DxsoBindingType::ConstantBuffer,
//     DxsoConstantBuffers::VSFixedFunction
// ) = 4
layout(set = 0, binding = 4, scalar, row_major) uniform ShaderData {
    D3D9FixedFunctionVS data;
};

layout(push_constant, scalar, row_major) uniform RenderStates {
    D3D9RenderStateInfo rs;
};


// Dynamic "spec constants"
// See d3d9_spec_constants.h for packing
// MaxSpecDwords = 6
// Binding has to match with getSpecConstantBufferSlot in dxso_util.h
layout(set = 0, binding = 31, scalar) uniform SpecConsts {
    uint dynamicSpecConstDword[13];
};


// Bindings have to match with computeResourceSlotId in dxso_util.h
// computeResourceSlotId(
//     DxsoProgramType::VertexShader,
//     DxsoBindingType::ConstantBuffer,
//     DxsoConstantBuffers::VSVertexBlendData
// ) = 5
layout(set = 0, binding = 5, std140, row_major) readonly buffer VertexBlendData {
    mat4 WorldViewArray[];
};


// Bindings have to match with computeResourceSlotId in dxso_util.h
// computeResourceSlotId(
//     DxsoProgramType::VertexShader,
//     DxsoBindingType::ConstantBuffer,
//     DxsoConstantBuffers::VSClipPlanes
// ) = 3
layout(set = 0, binding = 3, std140) uniform ClipPlanes {
    vec4 clipPlanes[MaxClipPlaneCount];
};


// Thanks SPIRV-Cross
spirv_instruction(set = "GLSL.std.450", id = 79) float spvNMin(float, float);
spirv_instruction(set = "GLSL.std.450", id = 79) vec2 spvNMin(vec2, vec2);
spirv_instruction(set = "GLSL.std.450", id = 79) vec3 spvNMin(vec3, vec3);
spirv_instruction(set = "GLSL.std.450", id = 79) vec4 spvNMin(vec4, vec4);
spirv_instruction(set = "GLSL.std.450", id = 81) float spvNClamp(float, float, float);
spirv_instruction(set = "GLSL.std.450", id = 81) vec2 spvNClamp(vec2, vec2, vec2);
spirv_instruction(set = "GLSL.std.450", id = 81) vec3 spvNClamp(vec3, vec3, vec3);
spirv_instruction(set = "GLSL.std.450", id = 81) vec4 spvNClamp(vec4, vec4, vec4);


// Functions to extract information from the packed VS key
// See D3D9FFShaderKeyVSData in d3d9_shader_types.h
// Please, dearest compiler, inline all of this.
uint texcoordIndices() {
    return bitfieldExtract(data.Key.Primitive[0], 0, 24);
}
bool vertexHasPositionT() {
    return bitfieldExtract(data.Key.Primitive[0], 24, 1) != 0;
}
bool vertexHasColor0() {
    return bitfieldExtract(data.Key.Primitive[0], 25, 1) != 0;
}
bool vertexHasColor1() {
    return bitfieldExtract(data.Key.Primitive[0], 26, 1) != 0;
}
bool vertexHasPointSize() {
    return bitfieldExtract(data.Key.Primitive[0], 27, 1) != 0;
}
bool useLighting() {
    return bitfieldExtract(data.Key.Primitive[0], 28, 1) != 0;
}
bool normalizeNormals() {
    return bitfieldExtract(data.Key.Primitive[0], 29, 1) != 0;
}
bool localViewer() {
    return bitfieldExtract(data.Key.Primitive[0], 30, 1) != 0;
}
bool rangeFog() {
    return bitfieldExtract(data.Key.Primitive[0], 31, 1) != 0;
}

uint texcoordFlags() {
    return bitfieldExtract(data.Key.Primitive[1], 0, 24);
}
uint diffuseSource() {
    return bitfieldExtract(data.Key.Primitive[1], 24, 2);
}
uint ambientSource() {
    return bitfieldExtract(data.Key.Primitive[1], 26, 2);
}
uint specularSource() {
    return bitfieldExtract(data.Key.Primitive[1], 28, 2);
}
uint emissiveSource() {
    return bitfieldExtract(data.Key.Primitive[1], 30, 2);
}

uint transformFlags() {
    return bitfieldExtract(data.Key.Primitive[2], 0, 24);
}
uint lightCount() {
    return bitfieldExtract(data.Key.Primitive[2], 24, 4);
}

uint vertexTexcoordDeclMask() {
    return bitfieldExtract(data.Key.Primitive[3], 0, 24);
}
bool vertexHasFog() {
    return bitfieldExtract(data.Key.Primitive[3], 24, 1) != 0;
}
D3D9FF_VertexBlendMode blendMode() {
    return bitfieldExtract(data.Key.Primitive[3], 25, 2);
}
bool vertexBlendIndexed() {
    return bitfieldExtract(data.Key.Primitive[3], 27, 1) != 0;
}
uint vertexBlendCount() {
    return bitfieldExtract(data.Key.Primitive[3], 28, 2);
}
bool vertexClipping() {
    return bitfieldExtract(data.Key.Primitive[3], 30, 1) != 0;
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
bool specSamplerDepthMode(uint textureStage) {
    uint dword = specIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 0 + int(textureStage), 1) != 0u;
}
uint specAlphaCompareOp() {
    uint dword = specIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 21, 3);
}
bool specAnySamplerProjected() {
    uint dword = specIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 24, 8) != 0u;
}
bool specSamplerProjected(uint textureStage) {
    uint dword = specIsOptimized() ? SpecConstDword1 : dynamicSpecConstDword[1];
    return bitfieldExtract(dword, 24 + int(textureStage), 1) != 0u;
}

bool specSamplerNull(uint textureStage) {
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
uint specVertexFogMode() {
    uint dword = specIsOptimized() ? SpecConstDword2 : dynamicSpecConstDword[2];
    return bitfieldExtract(dword, 26, 2);
}

bool specSamplerFetch4(uint textureStage) {
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



float calculateFog(vec4 vPos) {
    vec4 color1 = vertexHasColor1() ? in_Color1 : vec4(0.0);

    vec4 specular = color1;
    bool hasSpecular = vertexHasColor1();

    vec3 fogColor = vec3(rs.fogColor[0], rs.fogColor[1], rs.fogColor[2]);
    float fogScale = rs.fogScale;
    float fogEnd = rs.fogEnd;
    float fogDensity = rs.fogDensity;
    D3DFOGMODE fogMode = specVertexFogMode();
    bool fogEnabled = specFogEnabled();
    if (!fogEnabled) {
        return 0.0;
    }

    float w = vPos.w;
    float z = vPos.z;
    float depth;
    if (rangeFog()) {
        vec3 pos3 = vPos.xyz;
        depth = length(pos3);
    } else {
        depth = vertexHasFog() ? in_Fog : abs(z);
    }
    float fogFactor;
    if (vertexHasPositionT()) {
        fogFactor = hasSpecular ? specular.w : 1.0;
    } else {
        switch (fogMode) {
            case D3DFOG_NONE:
                if (hasSpecular)
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

    return fogFactor;
}


float calculatePointSize(vec4 vtx) {
    float value = vertexHasPointSize() ? in_PointSize : rs.pointSize;
    uint pointMode = specPointMode();
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
    uint clipPlaneCount = specClipPlaneCount();

    // Compute clip distances
    for (uint i = 0; i < MaxClipPlaneCount; i++) {
        vec4 clipPlane = clipPlanes[i];
        float dist = dot(worldPos, clipPlane);
        bool clipPlaneEnabled = i < clipPlaneCount;
        float value = clipPlaneEnabled ? dist : 0.0;
        gl_ClipDistance[i] = value;
    }
}


vec4 pickMaterialSource(uint source, vec4 material) {
    if (source == D3DMCS_MATERIAL)
        return material;
    else if (source == D3DMCS_COLOR1)
        return vertexHasColor0() ? in_Color0 : vec4(1.0);
    else
        return vertexHasColor1() ? in_Color1 : vec4(0.0);
}


void main() {
    vec4 vtx = in_Position0;
    gl_Position = in_Position0;
    vec3 normal = in_Normal0.xyz;

    if (blendMode() == D3D9FF_VertexBlendMode_Tween) {
        vec4 vtx1 = in_Position1;
        vec3 normal1 = in_Normal1.xyz;
        vtx = mix(vtx, vtx1, data.TweenFactor);
        normal = mix(normal, normal1, data.TweenFactor);
    }

    if (!vertexHasPositionT()) {
        if (blendMode() == D3D9FF_VertexBlendMode_Normal) {
            float blendWeightRemaining = 1.0;
            vec4 vtxSum = vec4(0.0);
            vec3 nrmSum = vec3(0.0);

            for (uint i = 0; i <= vertexBlendCount(); i++) {
                uint arrayIndex;
                if (vertexBlendIndexed()) {
                    arrayIndex = uint(round(in_BlendIndices[i]));
                } else {
                    arrayIndex = i;
                }
                mat4 worldView = WorldViewArray[arrayIndex];

                mat3 nrmMtx;
                for (uint j = 0; j < 3; j++) {
                    nrmMtx[j] = worldView[j].xyz;
                }

                vec4 vtxResult = vtx * worldView;
                vec3 nrmResult = normal * nrmMtx;

                float weight;
                if (i != vertexBlendCount()) {
                    weight = in_BlendWeight[i];
                    blendWeightRemaining -= weight;
                } else {
                    weight = blendWeightRemaining;
                }

                vec4 weightVec4 = vec4(weight, weight, weight, weight);

                vtxSum = fma(vtxResult, weightVec4, vtxSum);
                nrmSum = fma(nrmResult, weightVec4.xyz, nrmSum);
            }

            vtx = vtxSum;
            normal = nrmSum;
        } else {
            vtx = vtx * data.WorldView;

            mat3 nrmMtx = mat3(data.NormalMatrix);

            normal = nrmMtx * normal;
        }

        // Some games rely on normals not being normal.
        if (normalizeNormals()) {
            bool isZeroNormal = all(equal(normal, vec3(0.0, 0.0, 0.0)));
            normal = isZeroNormal ? normal : normalize(normal);
        }

        gl_Position = vtx * data.Projection;
    } else {
        gl_Position *= data.ViewportInfo.inverseExtent;
        gl_Position += data.ViewportInfo.inverseOffset;

        // We still need to account for perspective correction here...

        float w = gl_Position.w;
        float rhw = w == 0.0 ? 1.0 : 1.0 / w;
        gl_Position.xyz *= rhw;
        gl_Position.w = rhw;
    }

    vec4 outNrm = vec4(normal, 1.0);
    out_Normal = outNrm;

    vec4 texCoords[TextureStageCount];
    texCoords[0] = in_Texcoord0;
    texCoords[1] = in_Texcoord1;
    texCoords[2] = in_Texcoord2;
    texCoords[3] = in_Texcoord3;
    texCoords[4] = in_Texcoord4;
    texCoords[5] = in_Texcoord5;
    texCoords[6] = in_Texcoord6;
    texCoords[7] = in_Texcoord7;

    vec4 transformedTexCoords[TextureStageCount];

    for (uint i = 0; i < TextureStageCount; i++) {
        // 0b111 = 7
        uint inputIndex = (texcoordIndices() >> (i * 3)) & 7;
        uint inputFlags = (texcoordFlags() >> (i * 3)) & 7;
        uint texcoordCount = (vertexTexcoordDeclMask() >> (inputIndex * 3)) & 7;

        vec4 transformed;

        uint flags = (transformFlags() >> (i * 3)) & 7;

        // Passing 0xffffffff results in it getting clamped to the dimensions of the texture coords and getting treated as PROJECTED
        // but D3D9 does not apply the transformation matrix.
        bool applyTransform = flags > D3DTTFF_COUNT1 && flags <= D3DTTFF_COUNT4;

        uint count = min(flags, 4u);

        // A projection component index of 4 means we won't do projection
        uint projIndex = count != 0 ? count - 1 : 4;

        switch (inputFlags) {
            default:
            case (DXVK_TSS_TCI_PASSTHRU >> TCIOffset):
                transformed = texCoords[inputIndex & 0xFF];

                if (texcoordCount < 4) {
                    // Vulkan sets the w component to 1.0 if that's not provided by the vertex buffer, D3D9 expects 0 here
                    transformed.w = 0.0;
                }

                if (applyTransform && !vertexHasPositionT()) {
                    /*This doesn't happen every time and I cannot figure out the difference between when it does and doesn't.
                    Keep it disabled for now, it's more likely that games rely on the zero texcoord than the weird 1 here.
                    if (texcoordCount <= 1) {
                      // y gets padded to 1 for some reason
                      transformed.y = 1.0;
                    }*/

                    if (texcoordCount >= 1 && texcoordCount < 4) {
                        // The first component after the last one thats backed by a vertex buffer gets padded to 1 for some reason.
                        uint idx = texcoordCount;
                        transformed[idx] = 1.0;
                    }
                } else if (texcoordCount != 0 && !applyTransform) {
                    // COUNT0, COUNT1, COUNT > 4 => take count from vertex decl if that's not zero
                    count = texcoordCount;
                }

                projIndex = count != 0 ? count - 1 : 4;
                break;

            case (DXVK_TSS_TCI_CAMERASPACENORMAL >> TCIOffset):
                transformed = outNrm;
                if (!applyTransform) {
                    count = 3;
                    projIndex = 4;
                }
                break;

            case (DXVK_TSS_TCI_CAMERASPACEPOSITION >> TCIOffset):
                transformed = vtx;
                if (!applyTransform) {
                    count = 3;
                    projIndex = 4;
                }
                break;

            case (DXVK_TSS_TCI_CAMERASPACEREFLECTIONVECTOR >> TCIOffset): {
                vec3 vtx3 = vtx.xyz;
                vtx3 = normalize(vtx3);

                vec3 reflection = reflect(vtx3, normal);
                transformed = vec4(reflection, 1.0);
                if (!applyTransform) {
                    count = 3;
                    projIndex = 4;
                }
                break;
            }

            case (DXVK_TSS_TCI_SPHEREMAP >> TCIOffset): {
                vec3 vtx3 = vtx.xyz;
                vtx3 = normalize(vtx3);

                vec3 reflection = reflect(vtx3, normal);
                float m = length(reflection + vec3(0.0, 0.0, 1.0)) * 2.0;

                transformed = vec4(
                    reflection.x / m + 0.5,
                    reflection.y / m + 0.5,
                    0.0,
                    1.0
                );
                break;
            }
        }

        if (applyTransform && !vertexHasPositionT()) {
            transformed = transformed * data.TexcoordMatrices[i];
        }

        // TODO: Shouldn't projected be checked per texture stage?
        if (specAnySamplerProjected() && projIndex < 4) {
            // The projection idx is always based on the flags, even when the input mode is not DXVK_TSS_TCI_PASSTHRU.
            float projValue = transformed[projIndex];

            // The w component is only used for projection or unused, so always insert the component that's supposed to be divided by there.
            // The fragment shader will then decide whether to project or not.
            transformed.w = projValue;
        }

        // TODO: Shouldn't projected be checked per texture stage?
        uint totalComponents = (specAnySamplerProjected() && projIndex < 4) ? 3 : 4;
        for (uint j = count; j < totalComponents; j++) {
            // Discard the components that exceed the specified D3DTTFF_COUNT
            transformed[j] = 0.0;
        }

        transformedTexCoords[i] = transformed;
    }

    out_Texcoord0 = transformedTexCoords[0];
    out_Texcoord1 = transformedTexCoords[1];
    out_Texcoord2 = transformedTexCoords[2];
    out_Texcoord3 = transformedTexCoords[3];
    out_Texcoord4 = transformedTexCoords[4];
    out_Texcoord5 = transformedTexCoords[5];
    out_Texcoord6 = transformedTexCoords[6];
    out_Texcoord7 = transformedTexCoords[7];

    if (useLighting()) {
        vec4 diffuseValue = vec4(0.0);
        vec4 specularValue = vec4(0.0);
        vec4 ambientValue = vec4(0.0);

        for (uint i = 0; i < lightCount(); i++) {
            D3D9Light light = data.Lights[i];

            vec4 diffuse = light.Diffuse;
            vec4 specular = light.Specular;
            vec4 ambient = light.Ambient;
            vec3 position = light.Position.xyz;
            vec3 direction = light.Direction.xyz;
            uint type = light.Type;
            float range = light.Range;
            float falloff = light.Falloff;
            float atten0 = light.Attenuation0;
            float atten1 = light.Attenuation1;
            float atten2 = light.Attenuation2;
            float theta = light.Theta;
            float phi = light.Phi;

            bool isSpot = type == D3DLIGHT_SPOT;
            bool isDirectional = type == D3DLIGHT_DIRECTIONAL;

            bvec3 isDirectional3 = bvec3(isDirectional);

            vec3 vtx3 = vtx.xyz;

            vec3 delta = position - vtx3;
            float d = length(delta);
            vec3 hitDir = -direction;
                 hitDir = mix(delta, hitDir, isDirectional3);
                 hitDir = normalize(hitDir);

            float atten = fma(d, atten2, atten1);
                  atten = fma(d, atten, atten0);
                  atten = 1.0 / atten;
                  atten = spvNMin(atten, FLOAT_MAX_VALUE);

                  atten = d > range ? 0.0 : atten;
                  atten = isDirectional ? 1.0 : atten;

            // Spot Lighting
            {
                float rho = dot(-hitDir, direction);
                float spotAtten = rho - phi;
                      spotAtten = spotAtten / (theta - phi);
                      spotAtten = pow(spotAtten, falloff);

                bool insideThetaAndPhi = rho <= theta;
                bool insidePhi = rho > phi;
                     spotAtten = insidePhi ? spotAtten : 0.0;
                     spotAtten = insideThetaAndPhi ? spotAtten : 1.0;
                     spotAtten = clamp(spotAtten, 0.0, 1.0);

                     spotAtten = atten * spotAtten;
                     atten     = isSpot ? spotAtten : atten;
            }

            float hitDot = dot(normal, hitDir);
                  hitDot = clamp(hitDot, 0.0, 1.0);

            float diffuseness = hitDot * atten;

            vec3 mid;
            if (localViewer()) {
                mid = normalize(vtx3);
                mid = hitDir - mid;
            } else {
                mid = hitDir - vec3(0.0, 0.0, 1.0);
            }

            mid = normalize(mid);

            float midDot = dot(normal, mid);
                  midDot = clamp(midDot, 0.0, 1.0);
            bool doSpec = midDot > 0.0;
                 doSpec = doSpec && hitDot > 0.0;

            float specularness = pow(midDot, data.Material.Power);
                  specularness *= atten;
                  specularness = doSpec ? specularness : 0.0;

            vec4 lightAmbient  = ambient * atten;
            vec4 lightDiffuse  = diffuse * diffuseness;
            vec4 lightSpecular = specular * specularness;

            ambientValue  += lightAmbient;
            diffuseValue  += lightDiffuse;
            specularValue += lightSpecular;
        }

        vec4 matDiffuse  = pickMaterialSource(diffuseSource(), data.Material.Diffuse);
        vec4 matAmbient  = pickMaterialSource(ambientSource(), data.Material.Ambient);
        vec4 matEmissive = pickMaterialSource(emissiveSource(), data.Material.Emissive);
        vec4 matSpecular = pickMaterialSource(specularSource(), data.Material.Specular);

        vec4 finalColor0 = fma(matAmbient, data.GlobalAmbient, matEmissive);
             finalColor0 = fma(matAmbient, ambientValue, finalColor0);
             finalColor0 = fma(matDiffuse, diffuseValue, finalColor0);
             finalColor0.a = matDiffuse.a;

        vec4 finalColor1 = matSpecular * specularValue;

        // Saturate
        finalColor0 = clamp(finalColor0, vec4(0.0), vec4(1.0));

        finalColor1 = clamp(finalColor1, vec4(0.0), vec4(1.0));

        out_Color0 = finalColor0;
        out_Color1 = finalColor1;
    } else {
        out_Color0 = vertexHasColor0() ? in_Color0 : vec4(1.0);
        out_Color1 = vertexHasColor1() ? in_Color1 : vec4(0.0);
    }

    out_Fog = calculateFog(vtx);

    gl_PointSize = calculatePointSize(vtx);

    // We statically declare 6 clip planes, so we always need to write values.
    emitVsClipping(vtx);
}
