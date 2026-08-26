#include "minirvEMU.h"
#include <stdio.h>
#include <cassert>

// Helpers to hand-assemble instructions for the tests below.
static uint32_t enc_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
  return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}
static uint32_t enc_i(int32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
  return ((uint32_t)(imm & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}
static uint32_t enc_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) {
  uint32_t imm11_5 = (imm >> 5) & 0x7F;
  uint32_t imm4_0  = imm & 0x1F;
  return (imm11_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm4_0 << 7) | opcode;
}
static uint32_t enc_u(uint32_t imm20, uint32_t rd, uint32_t opcode) {
  return (imm20 << 12) | (rd << 7) | opcode;
}

static void run_n(int n) {
  for (int i = 0; i < n; i++) emu_step();
}

int main() {
  // ---- Regression: original addi/jalr test program ----
  emu_reset();
  emu_load_word(0x00, 0x01400513);
  emu_load_word(0x04, 0x010000e7);
  emu_load_word(0x08, 0x00c000e7);
  emu_load_word(0x0c, 0x00c00067);
  emu_load_word(0x10, 0x00a50513);
  emu_load_word(0x14, 0x00008067);
  run_n(6);
  assert(emu_get_pc() == 0x0c);
  assert(emu_get_reg(10) == 30);
  printf("[PASS] addi/jalr regression: a0=%d pc=0x%02x\n", emu_get_reg(10), emu_get_pc());

  // ---- add ----
  emu_reset();
  emu_load_word(0x00, enc_i(5, 0, 0b000, 1, 0b0010011));               // addi x1, x0, 5
  emu_load_word(0x04, enc_i(7, 0, 0b000, 2, 0b0010011));               // addi x2, x0, 7
  emu_load_word(0x08, enc_r(0b0000000, 2, 1, 0b000, 3, 0b0110011));    // add  x3, x1, x2
  run_n(3);
  assert(emu_get_reg(3) == 12);
  printf("[PASS] add: x3 = x1+x2 = %d\n", emu_get_reg(3));

  // ---- lui ----
  emu_reset();
  emu_load_word(0x00, enc_u(0xABCDE, 4, 0b0110111));                   // lui x4, 0xABCDE
  run_n(1);
  assert(emu_get_reg(4) == 0xABCDE000);
  printf("[PASS] lui: x4 = 0x%08x\n", emu_get_reg(4));

  // ---- addi with negative immediate (sign extension) ----
  emu_reset();
  emu_load_word(0x00, enc_i(-5, 0, 0b000, 5, 0b0010011));              // addi x5, x0, -5
  run_n(1);
  assert((int32_t)emu_get_reg(5) == -5);
  printf("[PASS] addi negative imm: x5 = %d\n", (int32_t)emu_get_reg(5));

  // ---- lw / lbu, per course hint (0x12345678 byte order check) ----
  emu_reset();
  emu_load_word(0x40, 0x12345678);                                    // test data
  emu_load_word(0x00, enc_i(0x40, 0, 0b000, 6, 0b0010011));            // addi x6, x0, 0x40 (base)
  emu_load_word(0x04, enc_i(0, 6, 0b010, 7, 0b0000011));               // lw   x7, 0(x6)
  emu_load_word(0x08, enc_i(0, 6, 0b100, 8, 0b0000011));               // lbu  x8, 0(x6)
  emu_load_word(0x0c, enc_i(1, 6, 0b100, 9, 0b0000011));               // lbu  x9, 1(x6)
  emu_load_word(0x10, enc_i(2, 6, 0b100, 10, 0b0000011));              // lbu  x10, 2(x6)
  emu_load_word(0x14, enc_i(3, 6, 0b100, 11, 0b0000011));              // lbu  x11, 3(x6)
  run_n(6);
  assert(emu_get_reg(7) == 0x12345678);
  assert(emu_get_reg(8) == 0x78);
  assert(emu_get_reg(9) == 0x56);
  assert(emu_get_reg(10) == 0x34);
  assert(emu_get_reg(11) == 0x12);
  printf("[PASS] lw/lbu: x7=0x%08x x8=0x%02x x9=0x%02x x10=0x%02x x11=0x%02x\n",
    emu_get_reg(7), emu_get_reg(8), emu_get_reg(9), emu_get_reg(10), emu_get_reg(11));

  // ---- sw ----
  emu_reset();
  emu_load_word(0x00, enc_i(0x50, 0, 0b000, 1, 0b0010011));            // addi x1, x0, 0x50 (base addr)
  emu_load_word(0x04, enc_u(0x12345, 2, 0b0110111));                   // lui x2, 0x12345      -> x2 = 0x12345000
  emu_load_word(0x08, enc_i(0x678, 2, 0b000, 2, 0b0010011));           // addi x2, x2, 0x678   -> x2 = 0x12345678
  emu_load_word(0x0c, enc_s(0, 2, 1, 0b010, 0b0100011));               // sw x2, 0(x1)
  emu_load_word(0x10, enc_i(0, 1, 0b010, 3, 0b0000011));               // lw x3, 0(x1)
  run_n(5);
  assert(emu_get_reg(3) == 0x12345678);
  printf("[PASS] sw: wrote and read back 0x%08x\n", emu_get_reg(3));

  // ---- sb, per course hint (reconstruct 0x90abcdef byte by byte) ----
  emu_reset();
  emu_load_word(0x40, 0x12345678);
  emu_load_word(0x00, enc_i(0x40, 0, 0b000, 12, 0b0010011));           // addi x12, x0, 0x40 (base)
  emu_load_word(0x04, enc_i(0x90, 0, 0b000, 1, 0b0010011));            // addi x1, x0, 0x90
  emu_load_word(0x08, enc_i(0xab, 0, 0b000, 2, 0b0010011));            // addi x2, x0, 0xab
  emu_load_word(0x0c, enc_i(0xcd, 0, 0b000, 3, 0b0010011));            // addi x3, x0, 0xcd
  emu_load_word(0x10, enc_i(0xef, 0, 0b000, 4, 0b0010011));            // addi x4, x0, 0xef
  emu_load_word(0x14, enc_s(3, 1, 12, 0b000, 0b0100011));              // sb x1, 3(x12)
  emu_load_word(0x18, enc_s(2, 2, 12, 0b000, 0b0100011));              // sb x2, 2(x12)
  emu_load_word(0x1c, enc_s(1, 3, 12, 0b000, 0b0100011));              // sb x3, 1(x12)
  emu_load_word(0x20, enc_s(0, 4, 12, 0b000, 0b0100011));              // sb x4, 0(x12)
  emu_load_word(0x24, enc_i(0, 12, 0b010, 5, 0b0000011));              // lw x5, 0(x12)
  run_n(10);
  assert(emu_get_reg(5) == 0x90abcdef);
  printf("[PASS] sb: reconstructed word = 0x%08x\n", emu_get_reg(5));


  // ---- ebreak / auto-termination ----
  emu_reset();
  emu_load_word(0x00, 0x01400513);           // addi a0, zero, 20
  emu_load_word(0x04, 0x010000e7);           // jalr ra, 16(zero)   -> fun
  emu_load_word(0x08, 0x00c000e7);           // jalr ra, 12(zero)   -> halt
  emu_load_word(0x0c, 0x00100073);           // ebreak (replaces the old spin-loop halt)
  emu_load_word(0x10, 0x00a50513);           // addi a0, a0, 10     -> fun
  emu_load_word(0x14, 0x00008067);           // jalr zero, 0(ra)    -> return
  int steps = 0;
  while (!emu_halted() && steps < 100) {
    emu_step();
    steps++;
  }
  assert(emu_halted());
  assert(emu_get_reg(10) == 30);
  printf("[PASS] ebreak auto-termination: a0=%d, halted after %d steps\n", emu_get_reg(10), steps);

  printf("\nAll minirvEMU tests PASSED.\n");
  return 0;
}
