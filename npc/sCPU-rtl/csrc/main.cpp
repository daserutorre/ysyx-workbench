#include "Vtop.h"
#include "verilated.h"
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

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  dut = new Vtop;

  reset(5);

  int cycle = 0;
  const int MAX_CYCLES = 40;
  while (dut->pc_out != 7 && cycle < MAX_CYCLES) {
    single_cycle();
    printf("cycle %2d: pc=%2d sum=%3d\n", cycle, dut->pc_out, dut->sum);
    cycle++;
  }

  printf("\nFinal: pc=%d sum=%d (after %d cycles)\n", dut->pc_out, dut->sum, cycle);
  assert(dut->pc_out == 7);
  assert(dut->sum == 55);
  printf("PASS: sum = 1+2+...+10 = %d, PC = %d\n", dut->sum, dut->pc_out);

  delete dut;
  return 0;
}
