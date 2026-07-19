#include "n64/vr4300.h"

#include <stdexcept>

namespace n64 {

namespace {

uint64_t SignExtend32(uint32_t value) {
  return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(value)));
}

int RegField(uint32_t instr, int shift) { return static_cast<int>((instr >> shift) & 0x1FU); }

}  // namespace

Vr4300::Vr4300(RdRam& rdram) : rdram_(rdram) {}

void Vr4300::Reset(uint32_t entry_point) {
  gpr_.fill(0);
  pc_ = entry_point;
  next_pc_ = entry_point + 4;
}

void Vr4300::set_gpr(int index, uint64_t value) {
  if (index == 0) {
    return;  // r0 is hardwired to zero.
  }
  gpr_[static_cast<size_t>(index)] = value;
}

uint32_t Vr4300::TranslateAddress(uint32_t vaddr) {
  // KSEG0 (cached) and KSEG1 (uncached) both map directly onto physical
  // memory by masking off the segment-select bits. No TLB is implemented,
  // so mapped (KUSEG/KSEG2/KSEG3) accesses are treated the same way for now.
  return vaddr & 0x1FFFFFFFU;
}

uint32_t Vr4300::Fetch(uint32_t vaddr) const { return rdram_.Read32(TranslateAddress(vaddr)); }

void Vr4300::Branch(bool condition, int32_t offset) {
  if (condition) {
    next_pc_ = static_cast<uint32_t>(static_cast<int32_t>(pc_) + 4 + (offset << 2));
  }
}

void Vr4300::Jump(uint32_t target) { next_pc_ = target; }

void Vr4300::Step() {
  const uint32_t instr = Fetch(pc_);
  const uint32_t new_pc = next_pc_;
  next_pc_ = next_pc_ + 4;
  Execute(instr);
  pc_ = new_pc;
}

void Vr4300::ExecuteSpecial(uint32_t instr) {
  const int rs = RegField(instr, 21);
  const int rt = RegField(instr, 16);
  const int rd = RegField(instr, 11);
  const uint32_t shamt = (instr >> 6) & 0x1FU;
  const uint32_t funct = instr & 0x3FU;
  const uint32_t rs_shift = static_cast<uint32_t>(gpr(rs)) & 0x1FU;

  switch (funct) {
    case 0x00:  // SLL
      set_gpr(rd, SignExtend32(static_cast<uint32_t>(gpr(rt)) << shamt));
      return;
    case 0x02:  // SRL
      set_gpr(rd, SignExtend32(static_cast<uint32_t>(gpr(rt)) >> shamt));
      return;
    case 0x03:  // SRA
      set_gpr(rd, SignExtend32(static_cast<uint32_t>(static_cast<int32_t>(gpr(rt)) >> shamt)));
      return;
    case 0x04:  // SLLV
      set_gpr(rd, SignExtend32(static_cast<uint32_t>(gpr(rt)) << rs_shift));
      return;
    case 0x06:  // SRLV
      set_gpr(rd, SignExtend32(static_cast<uint32_t>(gpr(rt)) >> rs_shift));
      return;
    case 0x07:  // SRAV
      set_gpr(rd, SignExtend32(static_cast<uint32_t>(static_cast<int32_t>(gpr(rt)) >> rs_shift)));
      return;
    case 0x08:  // JR
      Jump(static_cast<uint32_t>(gpr(rs)));
      return;
    case 0x09:  // JALR
      set_gpr(rd == 0 ? 31 : rd, pc_ + 8);
      Jump(static_cast<uint32_t>(gpr(rs)));
      return;
    case 0x20:  // ADD
    case 0x21:  // ADDU
      set_gpr(rd, SignExtend32(static_cast<uint32_t>(gpr(rs)) + static_cast<uint32_t>(gpr(rt))));
      return;
    case 0x22:  // SUB
    case 0x23:  // SUBU
      set_gpr(rd, SignExtend32(static_cast<uint32_t>(gpr(rs)) - static_cast<uint32_t>(gpr(rt))));
      return;
    case 0x24:  // AND
      set_gpr(rd, gpr(rs) & gpr(rt));
      return;
    case 0x25:  // OR
      set_gpr(rd, gpr(rs) | gpr(rt));
      return;
    case 0x26:  // XOR
      set_gpr(rd, gpr(rs) ^ gpr(rt));
      return;
    case 0x27:  // NOR
      set_gpr(rd, ~(gpr(rs) | gpr(rt)));
      return;
    case 0x2A:  // SLT
      set_gpr(rd, static_cast<int64_t>(gpr(rs)) < static_cast<int64_t>(gpr(rt)) ? 1U : 0U);
      return;
    case 0x2B:  // SLTU
      set_gpr(rd, gpr(rs) < gpr(rt) ? 1U : 0U);
      return;
    default:
      throw std::runtime_error("Unimplemented SPECIAL funct");
  }
}

void Vr4300::ExecuteRegImm(uint32_t instr) {
  const int rs = RegField(instr, 21);
  const uint32_t sub_opcode = (instr >> 16) & 0x1FU;
  const int32_t offset = static_cast<int16_t>(instr & 0xFFFFU);

  switch (sub_opcode) {
    case 0x00:  // BLTZ
      Branch(static_cast<int64_t>(gpr(rs)) < 0, offset);
      return;
    case 0x01:  // BGEZ
      Branch(static_cast<int64_t>(gpr(rs)) >= 0, offset);
      return;
    case 0x10:  // BLTZAL
      set_gpr(31, pc_ + 8);
      Branch(static_cast<int64_t>(gpr(rs)) < 0, offset);
      return;
    case 0x11:  // BGEZAL
      set_gpr(31, pc_ + 8);
      Branch(static_cast<int64_t>(gpr(rs)) >= 0, offset);
      return;
    default:
      throw std::runtime_error("Unimplemented REGIMM instruction");
  }
}

void Vr4300::Execute(uint32_t instr) {
  const uint32_t opcode = instr >> 26;
  const int rs = RegField(instr, 21);
  const int rt = RegField(instr, 16);
  const int32_t imm16 = static_cast<int16_t>(instr & 0xFFFFU);
  const auto addr_offset = static_cast<uint32_t>(imm16);
  const uint32_t uimm16 = instr & 0xFFFFU;

  switch (opcode) {
    case 0x00:
      ExecuteSpecial(instr);
      return;
    case 0x01:
      ExecuteRegImm(instr);
      return;
    case 0x02: {  // J
      const uint32_t target = ((pc_ + 4) & 0xF0000000U) | ((instr & 0x3FFFFFFU) << 2);
      Jump(target);
      return;
    }
    case 0x03: {  // JAL
      const uint32_t target = ((pc_ + 4) & 0xF0000000U) | ((instr & 0x3FFFFFFU) << 2);
      set_gpr(31, pc_ + 8);
      Jump(target);
      return;
    }
    case 0x04:  // BEQ
      Branch(gpr(rs) == gpr(rt), imm16);
      return;
    case 0x05:  // BNE
      Branch(gpr(rs) != gpr(rt), imm16);
      return;
    case 0x06:  // BLEZ
      Branch(static_cast<int64_t>(gpr(rs)) <= 0, imm16);
      return;
    case 0x07:  // BGTZ
      Branch(static_cast<int64_t>(gpr(rs)) > 0, imm16);
      return;
    case 0x08:  // ADDI
    case 0x09:  // ADDIU
      set_gpr(rt, SignExtend32(static_cast<uint32_t>(gpr(rs)) + addr_offset));
      return;
    case 0x0A:  // SLTI
      set_gpr(rt, static_cast<int64_t>(gpr(rs)) < imm16 ? 1U : 0U);
      return;
    case 0x0B:  // SLTIU
      set_gpr(rt, gpr(rs) < static_cast<uint64_t>(static_cast<int64_t>(imm16)) ? 1U : 0U);
      return;
    case 0x0C:  // ANDI
      set_gpr(rt, gpr(rs) & uimm16);
      return;
    case 0x0D:  // ORI
      set_gpr(rt, gpr(rs) | uimm16);
      return;
    case 0x0E:  // XORI
      set_gpr(rt, gpr(rs) ^ uimm16);
      return;
    case 0x0F:  // LUI
      set_gpr(rt, SignExtend32(uimm16 << 16));
      return;
    case 0x20:  // LB
      set_gpr(rt, static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(rdram_.Read8(
                      TranslateAddress(static_cast<uint32_t>(gpr(rs)) + addr_offset))))));
      return;
    case 0x21:  // LH
      set_gpr(rt, static_cast<uint64_t>(static_cast<int64_t>(static_cast<int16_t>(rdram_.Read16(
                      TranslateAddress(static_cast<uint32_t>(gpr(rs)) + addr_offset))))));
      return;
    case 0x23:  // LW
      set_gpr(rt, SignExtend32(rdram_.Read32(
                      TranslateAddress(static_cast<uint32_t>(gpr(rs)) + addr_offset))));
      return;
    case 0x24:  // LBU
      set_gpr(rt, rdram_.Read8(TranslateAddress(static_cast<uint32_t>(gpr(rs)) + addr_offset)));
      return;
    case 0x25:  // LHU
      set_gpr(rt, rdram_.Read16(TranslateAddress(static_cast<uint32_t>(gpr(rs)) + addr_offset)));
      return;
    case 0x28:  // SB
      rdram_.Write8(TranslateAddress(static_cast<uint32_t>(gpr(rs)) + addr_offset),
                    static_cast<uint8_t>(gpr(rt)));
      return;
    case 0x29:  // SH
      rdram_.Write16(TranslateAddress(static_cast<uint32_t>(gpr(rs)) + addr_offset),
                     static_cast<uint16_t>(gpr(rt)));
      return;
    case 0x2B:  // SW
      rdram_.Write32(TranslateAddress(static_cast<uint32_t>(gpr(rs)) + addr_offset),
                     static_cast<uint32_t>(gpr(rt)));
      return;
    default:
      throw std::runtime_error("Unimplemented opcode");
  }
}

}  // namespace n64
