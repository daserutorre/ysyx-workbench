#include "minirvEMU.h"
#include <stdio.h>
#include <string.h>

// RV32E: 16 GPRs, 32-bit PC. Memory is byte-addressable (matches RISC-V's
// own convention), so PC/addresses index M[] directly -- no width conversion needed.
#define MEM_SIZE 256

static uint32_t PC;
static uint32_t R[16];
static uint8_t  M[MEM_SIZE];
static int      halted;

static uint32_t fetch32(uint32_t addr) {
  // Little-endian 4-byte read.
  return (uint32_t)M[addr]        |
         (uint32_t)M[addr+1] << 8  |
         (uint32_t)M[addr+2] << 16 |
         (uint32_t)M[addr+3] << 24;
}

static void write32(uint32_t addr, uint32_t val) {
  M[addr]   = (uint8_t)(val);
  M[addr+1] = (uint8_t)(val >> 8);
  M[addr+2] = (uint8_t)(val >> 16);
  M[addr+3] = (uint8_t)(val >> 24);
}

static int32_t sext(uint32_t val, int bits) {
  int32_t shift = 32 - bits;
  return ((int32_t)(val << shift)) >> shift;
}

static void write_reg(int rd, uint32_t val) {
  if (rd != 0) R[rd] = val;  // x0 is hardwired to zero -- writes discarded
}

void emu_reset() {
  PC = 0;
  memset(R, 0, sizeof(R));
  memset(M, 0, sizeof(M));
  halted = 0;
}

void emu_load_word(uint32_t addr, uint32_t word) {
  write32(addr, word);
}

void emu_step() {
  if (halted) return;

  uint32_t inst = fetch32(PC);

  if (inst == 0x00100073) {
    // ebreak: stop the simulator.
    halted = 1;
    printf("HIT EBREAK at pc=0x%08x, program finished executing\n", PC);
    return;
  }

  uint32_t opcode = inst & 0x7F;
  uint32_t rd     = (inst >> 7) & 0x1F;
  uint32_t funct3 = (inst >> 12) & 0x7;
  uint32_t rs1    = (inst >> 15) & 0x1F;
  uint32_t rs2    = (inst >> 20) & 0x1F;
  uint32_t funct7 = (inst >> 25) & 0x7F;

  // I-type immediate (addi, lw, lbu, jalr): inst[31:20], sign-extended.
  int32_t imm_i = sext(inst >> 20, 12);

  // S-type immediate (sw, sb): inst[31:25] | inst[11:7], sign-extended.
  uint32_t imm_s_raw = ((inst >> 25) << 5) | ((inst >> 7) & 0x1F);
  int32_t imm_s = sext(imm_s_raw, 12);

  // U-type immediate (lui): inst[31:12] << 12 (already in upper-bit position).
  uint32_t imm_u = inst & 0xFFFFF000;

  if (opcode == 0b0110011 && funct3 == 0b000 && funct7 == 0b0000000) {
    // add: R[rd] = R[rs1] + R[rs2]
    write_reg(rd, R[rs1] + R[rs2]);
    PC += 4;
  } else if (opcode == 0b0010011 && funct3 == 0b000) {
    // addi: R[rd] = R[rs1] + sext(imm)
    write_reg(rd, R[rs1] + imm_i);
    PC += 4;
  } else if (opcode == 0b0110111) {
    // lui: R[rd] = imm[31:12] << 12
    write_reg(rd, imm_u);
    PC += 4;
  } else if (opcode == 0b0000011 && funct3 == 0b010) {
    // lw: R[rd] = sext(M[R[rs1]+imm][31:0])  -- already full-width, sext is a no-op here
    uint32_t addr = R[rs1] + imm_i;
    write_reg(rd, fetch32(addr));
    PC += 4;
  } else if (opcode == 0b0000011 && funct3 == 0b100) {
    // lbu: R[rd] = zext(M[R[rs1]+imm][7:0])
    uint32_t addr = R[rs1] + imm_i;
    write_reg(rd, (uint32_t)M[addr]);
    PC += 4;
  } else if (opcode == 0b0100011 && funct3 == 0b010) {
    // sw: M[R[rs1]+imm] = R[rs2][31:0]
    uint32_t addr = R[rs1] + imm_s;
    write32(addr, R[rs2]);
    PC += 4;
  } else if (opcode == 0b0100011 && funct3 == 0b000) {
    // sb: M[R[rs1]+imm] = R[rs2][7:0]
    uint32_t addr = R[rs1] + imm_s;
    M[addr] = (uint8_t)(R[rs2] & 0xFF);
    PC += 4;
  } else if (opcode == 0b1100111 && funct3 == 0b000) {
    // jalr: t = PC+4; PC = (R[rs1] + sext(imm)) & ~1; if (rd) R[rd] = t
    uint32_t target = (R[rs1] + imm_i) & ~1u;
    uint32_t link = PC + 4;
    PC = target;
    write_reg(rd, link);
  } else {
    // Unimplemented (e.g. ebreak, handled separately later) -- just advance PC.
    PC += 4;
  }
}

uint32_t emu_get_pc() {
  return PC;
}

uint32_t emu_get_reg(int i) {
  return R[i & 0xF];
}

int emu_halted() {
  return halted;
}
