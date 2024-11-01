#pragma once

#include "d3d9_include.h"
#include "d3d9_state.h"

namespace dxvk {

  class D3D9BoundResources final {

    public:

    D3D9BoundResources(const Direct3DState9& State);


    void UpdateTextureBitmasks(uint32_t index, DWORD combinedUsage);

    void UpdateActiveFetch4(uint32_t stateSampler);



    private:

    const Direct3DState9&           m_state;

    uint32_t                        m_depthTextures = 0;
    uint32_t                        m_drefClamp = 0;
    uint32_t                        m_cubeTextures = 0;
    uint32_t                        m_textureTypes = 0;
    uint32_t                        m_projectionBitfield  = 0;

    uint32_t                        m_dirtySamplerStates = 0;
    uint32_t                        m_dirtyTextures      = 0;

    uint32_t                        m_activeRTsWhichAreTextures : 4;
    uint32_t                        m_alphaSwizzleRTs : 4;
    uint32_t                        m_lastHazardsRT   : 4;

    uint32_t                        m_activeTextureRTs       = 0;
    uint32_t                        m_activeTextureDSs       = 0;
    uint32_t                        m_activeHazardsRT        = 0;
    uint32_t                        m_activeHazardsDS        = 0;
    uint32_t                        m_activeTextures         = 0;
    uint32_t                        m_activeTexturesToUpload = 0;
    uint32_t                        m_activeTexturesToGen    = 0;

    uint32_t                        m_activeVertexBuffers                = 0;
    uint32_t                        m_activeVertexBuffersToUpload        = 0;
    uint32_t                        m_activeVertexBuffersToUploadPerDraw = 0;

    // m_fetch4Enabled is whether fetch4 is currently enabled
    // from the application.
    //
    // m_fetch4 is whether it should be enabled in the shader
    // ie. are we in a correct state to use it
    // (enabled + texture supports it + point sampled)
    uint32_t                        m_fetch4Enabled = 0;
    uint32_t                        m_fetch4        = 0;

    uint32_t                        m_lastHazardsDS = 0;
    uint32_t                        m_lastSamplerTypesFF = 0;

  };

}
