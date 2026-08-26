#include "dpi.h"
#include <string.h>

#define MEM_SIZE 4096
static uint8_t pmem[MEM_SIZE];
static bool g_halted = false;

// ---- Plain C++ helpers (testbench-facing) ----

void pmem_load_word(uint32_t addr, uint32_t word) {
  pmem[addr]   = (uint8_t)(word);
  pmem[addr+1] = (uint8_t)(word >> 8);
  pmem[addr+2] = (uint8_t)(word >> 16);
  pmem[addr+3] = (uint8_t)(word >> 24);
}

bool npc_is_halted() {
  return g_halted;
}

// ---- DPI-C functions (RTL-facing, called directly from Verilog) ----

// Reads one 32-bit word at a word-aligned address.
extern "C" int pmem_read(int raddr) {
  uint32_t addr = ((uint32_t)raddr) & ~0x3u; // force word alignment, just in case
  return (int)((uint32_t)pmem[addr]        |
               (uint32_t)pmem[addr+1] << 8  |
               (uint32_t)pmem[addr+2] << 16 |
               (uint32_t)pmem[addr+3] << 24);
}

// Writes wdata into memory at a word-aligned address, only touching the
// byte lanes selected by wmask (bit i controls byte i, 1 = write that byte).
extern "C" void pmem_write(int waddr, int wdata, int wmask) {
  uint32_t addr = ((uint32_t)waddr) & ~0x3u;
  uint32_t data = (uint32_t)wdata;
  uint32_t mask = (uint32_t)wmask;

  if (mask & 0x1) pmem[addr]   = (uint8_t)(data);
  if (mask & 0x2) pmem[addr+1] = (uint8_t)(data >> 8);
  if (mask & 0x4) pmem[addr+2] = (uint8_t)(data >> 16);
  if (mask & 0x8) pmem[addr+3] = (uint8_t)(data >> 24);
}

extern "C" void npc_trap() {
  g_halted = true;
}
