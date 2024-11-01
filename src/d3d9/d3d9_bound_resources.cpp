#include "d3d9_bound_resources.h"
#include "d3d9_texture.h"
#include "d3d9_subresource.h"

namespace dxvk
{

  D3D9BoundResources::D3D9BoundResources(const Direct3DState9& State)
    : m_state(State) {
  }

  inline void D3D9BoundResources::UpdateTextureBitmasks(uint32_t index, DWORD combinedUsage) {
      const uint32_t bit = 1 << index;

      m_activeTextureRTs       &= ~bit;
      m_activeTextureDSs       &= ~bit;
      m_activeTextures         &= ~bit;
      m_activeTexturesToUpload &= ~bit;
      m_activeTexturesToGen    &= ~bit;

      auto tex = GetCommonTexture(m_state.textures[index]);
      if (tex != nullptr) {
        m_activeTextures |= bit;

        if (unlikely(tex->IsRenderTarget()))
          m_activeTextureRTs |= bit;

        if (unlikely(tex->IsDepthStencil()))
          m_activeTextureDSs |= bit;

        if (unlikely(tex->NeedsAnyUpload()))
          m_activeTexturesToUpload |= bit;

        if (unlikely(tex->NeedsMipGen()))
          m_activeTexturesToGen |= bit;

        // Update shadow sampler mask
        const bool oldDepth = m_depthTextures & bit;
        const bool newDepth = tex->IsShadow();

        if (oldDepth != newDepth) {
          m_depthTextures ^= bit;
          m_dirtySamplerStates |= bit;
        }

        // Update dref clamp mask
        m_drefClamp &= ~bit;
        m_drefClamp |= uint32_t(tex->IsUpgradedToD32f()) << index;

        const bool oldCube = m_cubeTextures & bit;
        const bool newCube = tex->GetType() == D3DRTYPE_CUBETEXTURE;
        if (oldCube != newCube) {
          m_cubeTextures ^= bit;
          m_dirtySamplerStates |= bit;
        }

        if (unlikely(m_fetch4Enabled & bit))
          UpdateActiveFetch4(index);
      } else {
        if (unlikely(m_fetch4 & bit))
          UpdateActiveFetch4(index);
      }

      if (unlikely(combinedUsage & D3DUSAGE_RENDERTARGET))
        UpdateActiveHazardsRT(bit);

      if (unlikely(combinedUsage & D3DUSAGE_DEPTHSTENCIL))
        UpdateActiveHazardsDS(bit);
    }

    void D3D9BoundResources::UpdateActiveFetch4(uint32_t stateSampler) {
      auto& state = m_state.samplerStates;

      const uint32_t samplerBit = 1u << stateSampler;

      auto texture = GetCommonTexture(m_state.textures[stateSampler]);
      const bool textureSupportsFetch4 = texture != nullptr && texture->SupportsFetch4();

      const bool fetch4Enabled = m_fetch4Enabled & samplerBit;
      const bool pointSampled  = state[stateSampler][D3DSAMP_MAGFILTER] == D3DTEXF_POINT;
      const bool shouldFetch4  = fetch4Enabled && textureSupportsFetch4 && pointSampled;

      if (unlikely(shouldFetch4 != !!(m_fetch4 & samplerBit))) {
        if (shouldFetch4)
          m_fetch4 |= samplerBit;
        else
          m_fetch4 &= ~samplerBit;
      }
    }

  }
