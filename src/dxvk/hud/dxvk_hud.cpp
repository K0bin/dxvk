#include <algorithm>
#include <cstring>

#include "dxvk_hud.h"

namespace dxvk::hud {
  
  Hud::Hud(
    const Rc<DxvkDevice>& device)
  : m_device        (device),
    m_renderer      (device),
    m_hudItems      (device) {
    util::SanitizeMxcsr("hud constructor 0");
    // Retrieve and sanitize options
    m_options.scale = std::clamp(m_hudItems.getOption<float>("scale", 1.0f), 0.25f, 4.0f);
    m_options.opacity = std::clamp(m_hudItems.getOption<float>("opacity", 1.0f), 0.1f, 1.0f);

    util::SanitizeMxcsr("hud constructor 1");
    addItem<HudVersionItem>("version", -1);
    util::SanitizeMxcsr("hud constructor 2");
    addItem<HudDeviceInfoItem>("devinfo", -1, m_device);
    util::SanitizeMxcsr("hud constructor 3");
    addItem<HudFpsItem>("fps", -1);
    util::SanitizeMxcsr("hud constructor 4");
    addItem<HudFrameTimeItem>("frametimes", -1, device, &m_renderer);
    util::SanitizeMxcsr("hud constructor 5");
    addItem<HudSubmissionStatsItem>("submissions", -1, device);
    util::SanitizeMxcsr("hud constructor 6");
    addItem<HudDrawCallStatsItem>("drawcalls", -1, device);
    util::SanitizeMxcsr("hud constructor 7");
    addItem<HudPipelineStatsItem>("pipelines", -1, device);
    util::SanitizeMxcsr("hud constructor 8");
    addItem<HudDescriptorStatsItem>("descriptors", -1, device);
    util::SanitizeMxcsr("hud constructor 9");
    addItem<HudMemoryStatsItem>("memory", -1, device);
    util::SanitizeMxcsr("hud constructor 10");
    addItem<HudMemoryDetailsItem>("allocations", -1, device, &m_renderer);
    util::SanitizeMxcsr("hud constructor 11");
    addItem<HudCsThreadItem>("cs", -1, device);
    util::SanitizeMxcsr("hud constructor 12");
    addItem<HudGpuLoadItem>("gpuload", -1, device);
    util::SanitizeMxcsr("hud constructor 13");
    addItem<HudCompilerActivityItem>("compiler", -1, device);
    util::SanitizeMxcsr("hud constructor 14");
  }


  Hud::~Hud() {
    
  }


  void Hud::update() {
    m_hudItems.update();
  }


  void Hud::render(
    const Rc<DxvkCommandList>&ctx,
    const Rc<DxvkImageView>&  dstView) {
    if (empty())
      return;

    auto key = m_renderer.getPipelineKey(dstView);

    m_renderer.beginFrame(ctx, dstView, m_options);
    m_hudItems.render(ctx, key, m_options, m_renderer);
    m_renderer.flushDraws(ctx, dstView, m_options);
    m_renderer.endFrame(ctx);
  }


  Rc<Hud> Hud::createHud(const Rc<DxvkDevice>& device) {
    return new Hud(device);
  }
  
}
