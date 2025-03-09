#include "dxso_decoder.h"

#include "dxso_tables.h"
#include <string>

namespace dxvk {

  bool DxsoSemantic::operator== (const DxsoSemantic& b) const {
    return usage == b.usage && usageIndex == b.usageIndex;
  }

  bool DxsoSemantic::operator!= (const DxsoSemantic& b) const {
    return usage != b.usage || usageIndex != b.usageIndex;
  }
  
  uint32_t DxsoDecodeContext::decodeInstructionLength(uint32_t token) {
    auto opcode = m_ctx.instruction.opcode;

    uint32_t length  = 0;
    const auto& info = this->getProgramInfo();

    // Comment ops have their own system for getting length.
    if (opcode == DxsoOpcode::Comment)
      return (token & 0x7fff0000) >> 16;

    if (opcode == DxsoOpcode::End)
      return 0;

    // SM2.0 and above has the length of the op in instruction count baked into it.
    // SM1.4 and below have fixed lengths and run off expectation.
    // Phase does not respect the following rules. :shrug:
    if (opcode != DxsoOpcode::Phase) {
      if (info.majorVersion() >= 2)
        length = (token & 0x0f000000) >> 24;
      else
        length = DxsoGetDefaultOpcodeLength(opcode);
    }

    // We've already logged this...
    if (length == InvalidOpcodeLength)
      return 0;

    // SM 1.4 has an extra param on Tex and TexCoord
    // As stated before, it also doesn't have the length of the op baked into the opcode
    if (info.majorVersion() == 1
     && info.minorVersion() == 4) {
      switch (opcode) {
        case DxsoOpcode::TexCoord:
        case DxsoOpcode::Tex:
          length += 1;
          break;
        default:
          break;
      }
    }

    return length;
  }

  bool DxsoDecodeContext::relativeAddressingUsesToken(
          DxsoInstructionArgumentType type) {
    auto& info = this->getProgramInfo();

    return (info.majorVersion() >= 2 && type == DxsoInstructionArgumentType::Source)
        || (info.majorVersion() >= 3 && type == DxsoInstructionArgumentType::Destination);
  }

  void DxsoDecodeContext::decodeDeclaration(DxsoCodeIter& iter) {
    uint32_t dclToken = iter.read();

    m_ctx.dcl.textureType          = static_cast<DxsoTextureType>((dclToken & 0x78000000) >> 27);
    m_ctx.dcl.semantic.usage       = static_cast<DxsoUsage>(dclToken & 0x0000000f);
    m_ctx.dcl.semantic.usageIndex  = (dclToken & 0x000f0000) >> 16;
  }

  void DxsoDecodeContext::decodeDefinition(DxsoOpcode opcode, DxsoCodeIter& iter) {
    const uint32_t instructionLength = std::min(m_ctx.instruction.tokenLength - 1, 4u);

    for (uint32_t i = 0; i < instructionLength; i++)
      m_ctx.def.uint32[i] = iter.read();
  }

  void DxsoDecodeContext::decodeBaseRegister(
            DxsoBaseRegister& reg,
            uint32_t          token) {
    reg.id.type = static_cast<DxsoRegisterType>(
        ((token & 0x00001800) >> 8)
      | ((token & 0x70000000) >> 28));

    reg.id.num = token & 0x000007ff;
  }

  void DxsoDecodeContext::decodeGenericRegister(
            DxsoRegister& reg,
            uint32_t      token) {
    this->decodeBaseRegister(reg, token);

    reg.hasRelative = (token & (1 << 13)) == 8192;
    reg.relative.id = DxsoRegisterId {
      DxsoRegisterType::Addr, 0 };
    reg.relative.swizzle = IdentitySwizzle;

    reg.centroid         = token & (4 << 20);
    reg.partialPrecision = token & (2 << 20);
  }

   void DxsoDecodeContext::decodeRelativeRegister(
            DxsoBaseRegister& reg,
            uint32_t          token) {
    this->decodeBaseRegister(reg, token);

    reg.swizzle = DxsoRegSwizzle(
      uint8_t((token & 0x00ff0000) >> 16));
  }

  bool DxsoDecodeContext::decodeDestinationRegister(DxsoCodeIter& iter) {
    uint32_t token = iter.read();

    this->decodeGenericRegister(m_ctx.dst, token);

    m_ctx.dst.mask = DxsoRegMask(
      uint8_t((token & 0x000f0000) >> 16));

    m_ctx.dst.saturate = (token & (1 << 20)) != 0;

    m_ctx.dst.shift    = (token & 0x0f000000) >> 24;
    m_ctx.dst.shift    = (m_ctx.dst.shift & 0x7) - (m_ctx.dst.shift & 0x8);

    const bool extraToken =
      relativeAddressingUsesToken(DxsoInstructionArgumentType::Destination);

    if (m_ctx.dst.hasRelative && extraToken) {
      this->decodeRelativeRegister(m_ctx.dst.relative, iter.read());
      return true;
    }

    return false;
  }

  bool DxsoDecodeContext::decodeSourceRegister(uint32_t i, DxsoCodeIter& iter) {
    if (i >= m_ctx.src.size())
      throw DxvkError("DxsoDecodeContext::decodeSourceRegister: source register out of range.");

    uint32_t token = iter.read();

    this->decodeGenericRegister(m_ctx.src[i], token);

    m_ctx.src[i].swizzle = DxsoRegSwizzle(
      uint8_t((token & 0x00ff0000) >> 16));

    m_ctx.src[i].modifier = static_cast<DxsoRegModifier>(
      (token & 0x0f000000) >> 24);

    const bool extraToken =
      relativeAddressingUsesToken(DxsoInstructionArgumentType::Source);

    if (m_ctx.src[i].hasRelative && extraToken) {
      this->decodeRelativeRegister(m_ctx.src[i].relative, iter.read());
      return true;
    }

    return false;
  }


  void DxsoDecodeContext::decodePredicateRegister(DxsoCodeIter& iter) {
    uint32_t token = iter.read();

    this->decodeGenericRegister(m_ctx.pred, token);

    m_ctx.pred.swizzle = DxsoRegSwizzle(
      uint8_t((token & 0x00ff0000) >> 16));

    m_ctx.pred.modifier = static_cast<DxsoRegModifier>(
      (token & 0x0f000000) >> 24);
  }


  bool DxsoDecodeContext::decodeInstruction(DxsoCodeIter& iter, const std::string& fileName) {
    uint32_t token = iter.read();

    m_ctx.instructionIdx++;

    m_ctx.instruction.opcode = static_cast<DxsoOpcode>(
      token & 0x0000ffff);

    m_ctx.instruction.predicated = token & (1 << 28);

    m_ctx.instruction.coissue    = token & 0x40000000;

    m_ctx.instruction.specificData.uint32 =
      (token & 0x00ff0000) >> 16;

    m_ctx.instruction.tokenLength =
      this->decodeInstructionLength(token);

    uint32_t tokenLength =
      m_ctx.instruction.tokenLength;

    std::string comment;
    std::wstring wcomment;

    switch (m_ctx.instruction.opcode) {
      case DxsoOpcode::If:
      case DxsoOpcode::Ifc:
      case DxsoOpcode::Rep:
      case DxsoOpcode::Loop:
      case DxsoOpcode::BreakC:
      case DxsoOpcode::BreakP: {
        uint32_t sourceIdx = 0;
        for (uint32_t i = 0; i < tokenLength; i++) {
          if (this->decodeSourceRegister(sourceIdx, iter))
            i++;

          sourceIdx++;
        }
        return true;
      }

      case DxsoOpcode::Dcl:
        this->decodeDeclaration(iter);
        this->decodeDestinationRegister(iter);
        return true;

      case DxsoOpcode::Def:
      case DxsoOpcode::DefI:
      case DxsoOpcode::DefB:
        this->decodeDestinationRegister(iter);
        this->decodeDefinition(
          m_ctx.instruction.opcode, iter);
        return true;

      case DxsoOpcode::Comment:
        comment = std::string(reinterpret_cast<const char*>(iter.ptrAt(0)));
        //Logger::warn(str::format("Comment: ", comment));

        //Logger::warn(str::format("Comment, filename: ", fileName));
        if (fileName == "VS_cee48b55a9031cada18a364e8841d5594bad5914") {
        //if (fileName == "VS_53a71dd90d40de6c0610fb94b43240893cddcd79") {
          //comment = std::string(reinterpret_cast<const char*>(iter.ptrAt(0)), tokenLength * sizeof(uint32_t));
          Logger::warn(str::format("Comment with token length: ", tokenLength));
          Logger::warn(str::format("Char 5: ", static_cast<uint32_t>(*(reinterpret_cast<const char*>(iter.ptrAt(0)) + 4))));
          //wcomment = std::wstring(reinterpret_cast<const wchar_t*>(iter.ptrAt(0)));
          //comment = std::string(wcomment.begin(), wcomment.end());
          Logger::warn(str::format("Comment with string length: ", comment.size()));
          Logger::warn(str::format("Comment: ", comment));
          std::vector<char> bytes;
          bytes.resize(tokenLength * 4 + 1, 0);
          memcpy(bytes.data(), iter.ptrAt(0), tokenLength * 4);


          struct Const {
            std::string name;
            uint32_t cIndex;
            uint32_t size;
            uint32_t typeSize;
          };
          // VS_53a71dd90d40de6c0610fb94b43240893cddcd79
          /*std::array<Const, 4> shaderConsts = {{
            { "VSC_WorldToScreen", 4, 4, 4 },
            { "VSC_CameraVectorForward", 18, 1, 1 },
            { "VSC_ShadowBiasParams", 29, 1, 1 },
            { "SkinningMatrices", 70, 180, 3 },
          }};*/


          // VS_cee48b55a9031cada18a364e8841d5594bad5914
          std::array<Const, 3> shaderConsts = {{
            { "VSC_LocalToWorld", 0, 3, 4 },
            { "VSC_WorldToScreen", 4, 4, 4 },
            { "VSC_WorldToView", 8, 3, 4 },
          }};

          struct Candidate {
            uint32_t nameOffset;
            std::vector<uint32_t> nameLengthWithoutNull;
            std::vector<uint32_t> nameLengthWithNull;
            std::vector<uint32_t> indexOffsets;
            std::vector<uint32_t> sizeOffsets;
            std::vector<uint32_t> sizeVec4sOffsets;
            std::vector<uint32_t> sizeFloatsOffsets;
            std::vector<uint32_t> nameOffsetOffsets;
          };
          std::unordered_map<uint32_t, Candidate> candidates;
          for (uint32_t i = 0; i < shaderConsts.size(); i++) {
            Candidate candidate = {};
            candidates.emplace(i, candidate);
          }

          for (uint32_t i = 1; i < tokenLength; i++) {
            //Logger::warn(str::format("Token: ", i, ": ", iter.at(i)));

            for (uint32_t byteI = 0; byteI < 4; byteI++) {
              uint32_t byteOffset = i * 4 + byteI;
              for (uint32_t shaderConstI = 0; shaderConstI < shaderConsts.size(); shaderConstI++) {
                const auto& shaderConst = shaderConsts[shaderConstI];

                auto stringlength = strlen(bytes.data() + i * 4 + byteI);
                if (stringlength == shaderConst.name.size()) {
                  std::string str(bytes.data() + byteOffset);
                  if (str == shaderConst.name) {
                    candidates[shaderConstI].nameOffset = byteOffset;
                  }
                }

                if (byteOffset % 1 == 0) {
                  auto c = static_cast<uint8_t>(*(bytes.data() + byteOffset));
                  if (c == shaderConst.name.size()) {
                    candidates[shaderConstI].nameLengthWithoutNull.push_back(byteOffset);
                  }
                  if (c == shaderConst.name.size() + 1) {
                    candidates[shaderConstI].nameLengthWithNull.push_back(byteOffset);
                  }
                  if (c == shaderConst.cIndex) {
                    candidates[shaderConstI].indexOffsets.push_back(byteOffset);
                  }
                  if (c == shaderConst.size) {
                    candidates[shaderConstI].sizeOffsets.push_back(byteOffset);
                  }
                  if (c == shaderConst.size / shaderConst.typeSize) {
                    candidates[shaderConstI].sizeVec4sOffsets.push_back(byteOffset);
                  }
                  if (c == shaderConst.size / (shaderConst.typeSize * 4)) {
                    candidates[shaderConstI].sizeFloatsOffsets.push_back(byteOffset);
                  }
                }
              }
            }
          }

          for (uint32_t i = 1; i < tokenLength; i++) {
            //Logger::warn(str::format("Token: ", i, ": ", iter.at(i)));

            for (uint32_t byteI = 0; byteI < 4; byteI++) {
              uint32_t byteOffset = i * 4 + byteI;
              for (uint32_t shaderConstI = 0; shaderConstI < shaderConsts.size(); shaderConstI++) {
                if (byteOffset % 4 == 0) {
                  auto c = static_cast<uint32_t>(*(bytes.data() + byteOffset));
                  c += 4u; // Because "CTAB" at the start
                  if (c == candidates[shaderConstI].nameOffset) {
                    candidates[shaderConstI].nameOffsetOffsets.push_back(byteOffset);
                  }
                }
              }
            }
          }

          uint32_t firstConstNameOffset = 99999;
          for (uint32_t shaderConstI = 0; shaderConstI < shaderConsts.size(); shaderConstI++) {
            const auto& candidate = candidates[shaderConstI];
            firstConstNameOffset = std::min(firstConstNameOffset, candidate.nameOffset);
          }

          //#define FIELDNAME indexOffsets
          //#define FIELDNAME sizeVec4sOffsets
          #define FIELDNAME nameOffsetOffsets

          std::unordered_set<uint32_t> constStrides;
          for (uint32_t shaderConstI = 0; shaderConstI < shaderConsts.size(); shaderConstI++) {
            const auto& candidate = candidates[shaderConstI];

            std::unordered_set<uint32_t> shaderConstStrides;
            for (uint32_t offset : candidate.FIELDNAME) {
              for (uint32_t shaderConstI2 = 0; shaderConstI2 < shaderConsts.size(); shaderConstI2++) {
                if (shaderConstI == shaderConstI2) {
                  continue;
                }
                const auto& candidateB = candidates[shaderConstI2];

                for (uint32_t offsetB : candidateB.FIELDNAME) {
                  uint32_t diff = uint32_t(std::abs(int32_t(offset) - int32_t(offsetB)));
                  if (diff == 0) {
                    continue;
                  }
                  shaderConstStrides.insert(diff);
                }
              }
            }
            for (auto iter = shaderConstStrides.begin(); iter != shaderConstStrides.end(); iter++) {
              Logger::warn(str::format("Const has possible strides ", *iter));
            }

            if (shaderConstI == 0) {
              for (auto iter = shaderConstStrides.begin(); iter != shaderConstStrides.end(); iter++) {
                Logger::warn(str::format("Inserting ", *iter));
                constStrides.insert(*iter);
              }
            } else {
              for (auto iter = constStrides.begin(); iter != constStrides.end(); iter++) {
                if (shaderConstStrides.find(*iter) == shaderConstStrides.end()) {
                  Logger::warn(str::format("Erasing ", *iter));
                  constStrides.erase(iter);
                }
              }
            }
          }
          for (auto iter = constStrides.begin(); iter != constStrides.end(); iter++) {
            for (auto iterB = constStrides.begin(); iterB != constStrides.end(); iterB++) {
              if (*iterB > *iter && ((*iterB) % (*iter)) == 0) {
                constStrides.erase(iterB);
              }
            }
          }
          Logger::warn(str::format("Stride candidates"));
          for (uint32_t stride : constStrides) {
            Logger::warn(str::format("Stride candidate: ", stride));
          }

          for (uint32_t shaderConstI = 0; shaderConstI < shaderConsts.size(); shaderConstI++) {
            const auto& shaderConst = shaderConsts[shaderConstI];
            const auto& candidate = candidates[shaderConstI];
            Logger::warn(str::format("Shader constant: ", shaderConst.name));
            uint32_t nameEnd = candidate.nameOffset + shaderConst.name.size() + 1;
            uint32_t alignedNameEnd = align(nameEnd, 4);
            Logger::warn(str::format("Found name at offset: ", candidate.nameOffset, " ends at byte: ", nameEnd, " aligned end: ", alignedNameEnd));
            for (auto offset : candidate.nameLengthWithNull) {
              int32_t relativeOffset = int32_t(offset) - int32_t(candidate.nameOffset);
              int32_t relativeOffsetToEnd = int32_t(offset) - int32_t(alignedNameEnd);
              int32_t relativeToFirstName = int32_t(offset) - int32_t(firstConstNameOffset);
              Logger::warn(str::format("Found name length with null at offset: ", offset, " Relative to name: ", relativeOffset, " Relative to name end: ", relativeOffsetToEnd, " Relative to first name: ", relativeToFirstName));
            }
            for (auto offset : candidate.nameLengthWithoutNull) {
              int32_t relativeOffset = int32_t(offset) - int32_t(candidate.nameOffset);
              int32_t relativeOffsetToEnd = int32_t(offset) - int32_t(alignedNameEnd);
              int32_t relativeToFirstName = int32_t(offset) - int32_t(firstConstNameOffset);
              Logger::warn(str::format("Found name length without null at offset: ", offset, " Relative to name: ", relativeOffset, " Relative to name end: ", relativeOffsetToEnd, " Relative to first name: ", relativeToFirstName));
            }
            for (auto offset : candidate.indexOffsets) {
              int32_t relativeOffset = int32_t(offset) - int32_t(candidate.nameOffset);
              int32_t relativeOffsetToEnd = int32_t(offset) - int32_t(alignedNameEnd);
              int32_t relativeToFirstName = int32_t(offset) - int32_t(firstConstNameOffset);
              Logger::warn(str::format("Found index at offset: ", offset, " Relative to name: ", relativeOffset, " Relative to name end: ", relativeOffsetToEnd, " Relative to first name: ", relativeToFirstName));
            }
            for (auto offset : candidate.sizeOffsets) {
              int32_t relativeOffset = int32_t(offset) - int32_t(candidate.nameOffset);
              int32_t relativeOffsetToEnd = int32_t(offset) - int32_t(alignedNameEnd);
              int32_t relativeToFirstName = int32_t(offset) - int32_t(firstConstNameOffset);
              Logger::warn(str::format("Found size at offset: ", offset, " Relative to name: ", relativeOffset, " Relative to name end: ", relativeOffsetToEnd, " Relative to first name: ", relativeToFirstName));
            }
            for (auto offset : candidate.sizeVec4sOffsets) {
              int32_t relativeOffset = int32_t(offset) - int32_t(candidate.nameOffset);
              int32_t relativeOffsetToEnd = int32_t(offset) - int32_t(alignedNameEnd);
              int32_t relativeToFirstName = int32_t(offset) - int32_t(firstConstNameOffset);
              Logger::warn(str::format("Found size in vec4s at offset: ", offset, " Relative to name: ", relativeOffset, " Relative to name end: ", relativeOffsetToEnd, " Relative to first name: ", relativeToFirstName));
            }
            for (auto offset : candidate.sizeFloatsOffsets) {
              int32_t relativeOffset = int32_t(offset) - int32_t(candidate.nameOffset);
              int32_t relativeOffsetToEnd = int32_t(offset) - int32_t(alignedNameEnd);
              int32_t relativeToFirstName = int32_t(offset) - int32_t(firstConstNameOffset);
              Logger::warn(str::format("Found size in floats at offset: ", offset, " Relative to name: ", relativeOffset, " Relative to name end: ", relativeOffsetToEnd, " Relative to first name: ", relativeToFirstName));
            }
            for (auto offset : candidate.nameOffsetOffsets) {
              int32_t relativeOffset = int32_t(offset) - int32_t(candidate.nameOffset);
              int32_t relativeOffsetToEnd = int32_t(offset) - int32_t(alignedNameEnd);
              int32_t relativeToFirstName = int32_t(offset) - int32_t(firstConstNameOffset);
              Logger::warn(str::format("Found name offset at offset: ", offset, " Relative to name: ", relativeOffset, " Relative to name end: ", relativeOffsetToEnd, " Relative to first name: ", relativeToFirstName));
            }
          }

        }
        iter = iter.skip(tokenLength);
        return true;

      default: {
        uint32_t sourceIdx = 0;
        for (uint32_t i = 0; i < tokenLength; i++) {
          if (i == 0) {
            if (this->decodeDestinationRegister(iter))
              i++;
          }
          else if (i == 1 && m_ctx.instruction.predicated) {
            // Relative addressing makes no sense
            // for predicate registers.
            this->decodePredicateRegister(iter);
          }
          else {
            if (this->decodeSourceRegister(sourceIdx, iter))
              i++;

            sourceIdx++;
          }
        }
        return true;
      }

      case DxsoOpcode::End:
        return false;
    }
  }

  std::ostream& operator << (std::ostream& os, DxsoUsage usage) {
    switch (usage) {
      case DxsoUsage::Position:     os << "Position"; break;
      case DxsoUsage::BlendWeight:  os << "BlendWeight"; break;
      case DxsoUsage::BlendIndices: os << "BlendIndices"; break;
      case DxsoUsage::Normal:       os << "Normal"; break;
      case DxsoUsage::PointSize:    os << "PointSize"; break;
      case DxsoUsage::Texcoord:     os << "Texcoord"; break;
      case DxsoUsage::Tangent:      os << "Tangent"; break;
      case DxsoUsage::Binormal:     os << "Binormal"; break;
      case DxsoUsage::TessFactor:   os << "TessFactor"; break;
      case DxsoUsage::PositionT:    os << "PositionT"; break;
      case DxsoUsage::Color:        os << "Color"; break;
      case DxsoUsage::Fog:          os << "Fog"; break;
      case DxsoUsage::Depth:        os << "Depth"; break;
      case DxsoUsage::Sample:       os << "Sample"; break;
      default:
        os << "Invalid Format (" << static_cast<uint32_t>(usage) << ")"; break;
    }

    return os;
  }

}