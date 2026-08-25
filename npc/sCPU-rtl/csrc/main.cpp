#include "Vtop.h"
#include "verilated.h"
#include "sEMU.h"
#include <stdio.h>
#include <cassert>

static Vtop *dut;

static void dut_single_cycle() {
  dut->clk = 0; dut->eval();
  dut->clk = 1; dut->eval();
}

static void dut_reset(int n) {
  dut->rst = 1;
  while (n-- > 0) dut_single_cycle();
  dut->rst = 0;
}

static int check_regs(int step) {
  int is_diff = 0;
  if (dut->pc_out != ref_get_pc()) {
    printf("[DIFF] step %d: PC differs: dut=%d ref=%d\n", step, dut->pc_out, ref_get_pc());
    is_diff = 1;
  }
  if (dut->r0_out != ref_get_reg(0)) {
    printf("[DIFF] step %d: R0 differs: dut=%d ref=%d\n", step, dut->r0_out, ref_get_reg(0));
    is_diff = 1;
  }
  if (dut->r1_out != ref_get_reg(1)) {
    printf("[DIFF] step %d: R1 differs: dut=%d ref=%d\n", step, dut->r1_out, ref_get_reg(1));
    is_diff = 1;
  }
  if (dut->r2_out != ref_get_reg(2)) {
    printf("[DIFF] step %d: R2 differs: dut=%d ref=%d\n", step, dut->r2_out, ref_get_reg(2));
    is_diff = 1;
  }
  if (dut->r3_out != ref_get_reg(3)) {
    printf("[DIFF] step %d: R3 differs: dut=%d ref=%d\n", step, dut->r3_out, ref_get_reg(3));
    is_diff = 1;
  }
  return is_diff;
}

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  dut = new Vtop;

  dut_reset(5);
  ref_reset();

  int step = 0;
  const int MAX_STEPS = 40;
  int diff_found = 0;

  while (dut->pc_out != 7 && step < MAX_STEPS) {
    dut_single_cycle();
    ref_step();

    printf("step %2d: dut(pc=%2d r0=%3d r1=%3d r2=%3d r3=%3d)  ref(pc=%2d r0=%3d r1=%3d r2=%3d r3=%3d)\n",
      step, dut->pc_out, dut->r0_out, dut->r1_out, dut->r2_out, dut->r3_out,
      ref_get_pc(), ref_get_reg(0), ref_get_reg(1), ref_get_reg(2), ref_get_reg(3));

    if (check_regs(step)) {
      printf("DiffTest FAILED at step %d\n", step);
      diff_found = 1;
      break;
    }
    step++;
  }

  if (!diff_found) {
    printf("\nDiffTest PASSED: dut and ref agree for all %d steps\n", step);
    printf("Final: pc=%d r2(sum)=%d\n", dut->pc_out, dut->r2_out);
    assert(dut->pc_out == 7);
    assert(dut->r2_out == 55);
    printf("PASS: sum = 1+2+...+10 = %d, PC = %d\n", dut->r2_out, dut->pc_out);
  }

  delete dut;
  return diff_found ? 1 : 0;
}
