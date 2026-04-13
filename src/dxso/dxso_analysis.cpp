#include "dxso_analysis.h"

namespace dxvk {

  DxsoAnalyzer::DxsoAnalyzer(
    DxsoAnalysisInfo& analysis)
    : m_analysis(&analysis) { }

  void DxsoAnalyzer::processInstruction(
    const DxsoInstructionContext& ctx) {
    DxsoOpcode opcode = ctx.instruction.opcode;

    // Co-issued CNDs are issued before their parents,
    // except when the parent is a CND.
    if (opcode == DxsoOpcode::Cnd &&
        m_parentOpcode != DxsoOpcode::Cnd &&
        ctx.instruction.coissue) {
      m_analysis->coissues.push_back(ctx);
    }

    m_parentOpcode = ctx.instruction.opcode;
  }

  void DxsoAnalyzer::finalize(size_t tokenCount) {
    m_analysis->bytecodeByteLength = tokenCount * sizeof(uint32_t);
  }

}