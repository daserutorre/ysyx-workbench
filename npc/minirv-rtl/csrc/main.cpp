#include "Vtop.h"
#include "verilated.h"
#include "dpi.h"
#include <stdio.h>
#include <cassert>

static Vtop *dut;

static void single_cycle() {
  dut->clk = 0; dut->eval();
  dut->clk = 1; dut->eval();
  npc_tick();
}

static void reset(int n) {
  dut->rst = 1;
  while (n-- > 0) single_cycle();
  dut->rst = 0;
}

static uint32_t dbg_read(int reg) {
  dut->dbg_addr = reg;
  dut->eval();
  return dut->dbg_data;
}

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  dut = new Vtop;

  if (argc < 2) {
    printf("Usage: %s <path-to-bin-file>\n", argv[0]);
    return 1;
  }

  if (!pmem_load_file(argv[1])) {
    printf("Failed to load image, aborting.\n");
    return 1;
  }

  reset(2);

  int steps = 0;
  const int MAX_STEPS = 10000000;
  const int TRACE_STEPS = 20;
  while (!npc_is_halted() && steps < MAX_STEPS) {
    single_cycle();
    steps++;
  }

  if (!npc_is_halted()) {
    printf("Simulation did not halt within %d steps (no ebreak hit).\n", MAX_STEPS);
    delete dut;
    return 1;
  }

  uint32_t a0 = dbg_read(10);
  uint64_t cycles = npc_get_cycle_count();
  uint64_t instret = npc_get_instret_count();
  double ipc = cycles > 0 ? (double)instret / (double)cycles : 0.0;

  printf("\nHalted after %d steps. pc=0x%08x a0=%u\n", steps, dut->pc, a0);
  printf("cycles=%llu instret=%llu IPC=%.3f\n",
    (unsigned long long)cycles, (unsigned long long)instret, ipc);

  delete dut;
  return 0;
}
