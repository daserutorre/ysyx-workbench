#include "Vtop.h"
#include "verilated.h"
#include "dpi.h"
#include "minirvEMU.h"
#include <stdio.h>
#include <cassert>

static Vtop *dut;

static void single_cycle() {
  dut->clk = 0; dut->eval();
  dut->clk = 1; dut->eval();
}

static void reset(int n) {
  dut->rst = 1;
  while (n-- > 0) single_cycle();
  dut->rst = 0;
}

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

static uint32_t dbg_read(int reg) {
  dut->dbg_addr = reg;
  dut->eval();
  return dut->dbg_data;
}

static void load_both(uint32_t addr, uint32_t word) {
  pmem_load_word(addr, word);  // dut's memory (via DPI-C, in dpi.cpp)
  emu_load_word(addr, word);   // ref's memory (inside minirvEMU.cpp)
}

static int check_regs(int step) {
  int is_diff = 0;

  if (dut->pc != emu_get_pc()) {
    printf("[DIFF] step %d: PC differs: dut=0x%x ref=0x%x\n", step, dut->pc, emu_get_pc());
    is_diff = 1;
  }
  for (int i = 0; i < 16; i++) {
    uint32_t dut_val = dbg_read(i);
    uint32_t ref_val = emu_get_reg(i);
    if (dut_val != ref_val) {
      printf("[DIFF] step %d: x%d differs: dut=0x%x ref=0x%x\n", step, i, dut_val, ref_val);
      is_diff = 1;
    }
  }
  return is_diff;
}

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  dut = new Vtop;
  emu_reset();

  // Program exercising all 8 instructions:
  //  0: addi x1, x0, 5
  //  4: addi x2, x0, 7
  //  8: add  x3, x1, x2
  //  c: lui  x4, 0x40
  // 10: addi x5, x0, 0x40
  // 14: sw   x3, 0(x5)
  // 18: lw   x6, 0(x5)
  // 1c: addi x7, x0, 0xAB
  // 20: sb   x7, 4(x5)
  // 24: lbu  x8, 4(x5)
  // 28: ebreak
  load_both(0x00, enc_i(5, 0, 0b000, 1, 0b0010011));
  load_both(0x04, enc_i(7, 0, 0b000, 2, 0b0010011));
  load_both(0x08, enc_r(0b0000000, 2, 1, 0b000, 3, 0b0110011));
  load_both(0x0c, enc_u(0x40, 4, 0b0110111));
  load_both(0x10, enc_i(0x40, 0, 0b000, 5, 0b0010011));
  load_both(0x14, enc_s(0, 3, 5, 0b010, 0b0100011));
  load_both(0x18, enc_i(0, 5, 0b010, 6, 0b0000011));
  load_both(0x1c, enc_i(0xAB, 0, 0b000, 7, 0b0010011));
  load_both(0x20, enc_s(4, 7, 5, 0b000, 0b0100011));
  load_both(0x24, enc_i(4, 5, 0b100, 8, 0b0000011));
  load_both(0x28, 0x00100073); // ebreak

  reset(2);

  int step = 0;
  const int MAX_STEPS = 30;
  int diff_found = 0;

  while (!npc_is_halted() && !emu_halted() && step < MAX_STEPS) {
    single_cycle();
    emu_step();
    step++;

    printf("step %2d: dut_pc=0x%02x ref_pc=0x%02x dut_halted=%d ref_halted=%d\n",
      step, dut->pc, emu_get_pc(), npc_is_halted(), emu_halted());

    if (check_regs(step)) {
      printf("DiffTest FAILED at step %d\n", step);
      diff_found = 1;
      break;
    }
  }

  if (!diff_found) {
    assert(npc_is_halted() && emu_halted());
    printf("\nDiffTest PASSED: dut and ref agree for all %d steps\n", step);
    printf("x3=%d x4=0x%x x6=%d x8=0x%x\n", dbg_read(3), dbg_read(4), dbg_read(6), dbg_read(8));
    printf("PASS\n");
  }

  delete dut;
  return diff_found ? 1 : 0;
}
