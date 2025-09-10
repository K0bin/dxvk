#include "dxso_util.h"

#include "dxso_include.h"

namespace dxvk {

  dxvk::mutex                  g_linkerSlotMutex;
  uint32_t                     g_linkerSlotCount = 12;
  std::array<DxsoSemantic, 32> g_linkerSlots = {
    {
      {DxsoUsage::Normal,   0},
      {DxsoUsage::Texcoord,   0},
      {DxsoUsage::Texcoord,   1},
      {DxsoUsage::Texcoord,   2},
      {DxsoUsage::Texcoord,   3},
      {DxsoUsage::Texcoord,   4},
      {DxsoUsage::Texcoord,   5},
      {DxsoUsage::Texcoord,   6},
      {DxsoUsage::Texcoord,   7},

      {DxsoUsage::Color,      0},
      {DxsoUsage::Color,      1},

      {DxsoUsage::Fog,        0},
    }};
  // We set fixed locations for the outputs that fixed function vertex shaders
  // can produce so the uber shader doesn't need to be patched at runtime.

  std::array<uint32_t, 14> g_semanticCounts = {};

  uint32_t RegisterLinkerSlot(DxsoSemantic semantic, bool isSM3) {
    // Lock, because games could be trying
    // to make multiple shaders at a time.
    std::lock_guard<dxvk::mutex> lock(g_linkerSlotMutex);

    // Need to choose a slot that maps nicely and similarly
    // between vertex and pixel shaders

    if (isSM3) {
      g_semanticCounts[static_cast<uint32_t>(semantic.usage)] = std::max(g_semanticCounts[static_cast<uint32_t>(semantic.usage)], semantic.usageIndex + 1);

      auto usageToString = [](DxsoUsage usage) {
        switch (usage) {
          case DxsoUsage::Position: return "Position";
          case DxsoUsage::BlendWeight: return "BlendWeight";
          case DxsoUsage::BlendIndices: return "BlendIndices";
          case DxsoUsage::Normal: return "Normal";
          case DxsoUsage::PointSize: return "PointSize";
          case DxsoUsage::Texcoord: return "Texcoord";
          case DxsoUsage::Tangent: return "Tangent";
          case DxsoUsage::Binormal: return "Binormal";
          case DxsoUsage::TessFactor: return "TessFactor";
          case DxsoUsage::PositionT: return "PositionT";
          case DxsoUsage::Color: return "Color";
          case DxsoUsage::Fog: return "Fog";
          case DxsoUsage::Depth: return "Depth";
          case DxsoUsage::Sample: return "Sample";
          default: return "Unknown";
        }
      };

      Logger::warn("============================");
      Logger::warn(str::format("Registered semantic. Usage: ", usageToString(semantic.usage), ", Index: ", semantic.usageIndex));
      for (uint32_t i = 0; i < g_semanticCounts.size(); i++) {
        if (g_semanticCounts[i] == 0) continue;
        Logger::warn(str::format("Usage: ", usageToString(static_cast<DxsoUsage>(i)), ", Highest index: ", g_semanticCounts[i] - 1));
      }
    }

    // Find or map a slot.
    uint32_t slot = g_linkerSlotCount;
    for (uint32_t j = 0; j < g_linkerSlotCount; j++) {
      if (g_linkerSlots[j] == semantic) {
        slot = j;
        break;
      }
    }

    if (slot == g_linkerSlotCount)
      g_linkerSlots[g_linkerSlotCount++] = semantic;

    return slot;
  }

}