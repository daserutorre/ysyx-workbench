#include "dpi.h"
#include <string.h>
#include <stdio.h>

#define MEM_BASE  0x80000000u
#define MEM_SIZE  (128 * 1024 * 1024) // 128MB
static uint8_t *pmem = nullptr;
static bool g_halted = false;

static uint8_t *get_pmem() {
  if (!pmem) {
    pmem = new uint8_t[MEM_SIZE];
    memset(pmem, 0, MEM_SIZE);
  }
  return pmem;
}

// ---- Plain C++ helpers (testbench-facing) ----

void pmem_load_word(uint32_t addr, uint32_t word) {
  uint8_t *m = get_pmem();
  uint32_t off = addr - MEM_BASE;
  m[off]   = (uint8_t)(word);
  m[off+1] = (uint8_t)(word >> 8);
  m[off+2] = (uint8_t)(word >> 16);
  m[off+3] = (uint8_t)(word >> 24);
}

bool pmem_load_file(const char *path) {
  uint8_t *m = get_pmem();
  FILE *fp = fopen(path, "rb");
  if (!fp) {
    printf("Failed to open image file: %s\n", path);
    return false;
  }
  size_t n = fread(m, 1, MEM_SIZE, fp);
  fclose(fp);
  printf("Loaded %zu bytes from %s\n", n, path);
  return true;
}

bool npc_is_halted() {
  return g_halted;
}

// ---- DPI-C functions (RTL-facing, called directly from Verilog) ----

// Reads one 32-bit word at a word-aligned address.
extern "C" int pmem_read(int raddr) {
  uint8_t *m = get_pmem();
  uint32_t addr = ((uint32_t)raddr) & ~0x3u; // force word alignment
  uint32_t off = addr - MEM_BASE;
  if (off >= MEM_SIZE) {
    // Reads are side-effect-free, and this wire is evaluated every cycle
    // regardless of whether the current instruction actually needs it
    // (e.g. the load-address wire in top.v computes unconditionally).
    // During reset, registers/PC/immediates are still settling, so
    // transient out-of-range addresses here are expected and harmless --
    // stay silent rather than spamming the log.
    return 0;
  }
  return (int)((uint32_t)m[off]        |
               (uint32_t)m[off+1] << 8  |
               (uint32_t)m[off+2] << 16 |
               (uint32_t)m[off+3] << 24);
}

// Writes wdata into memory at a word-aligned address, only touching the
// byte lanes selected by wmask (bit i controls byte i, 1 = write that byte).
extern "C" void pmem_write(int waddr, int wdata, int wmask) {
  uint8_t *m = get_pmem();
  uint32_t addr = ((uint32_t)waddr) & ~0x3u;
  uint32_t off = addr - MEM_BASE;
  uint32_t data = (uint32_t)wdata;
  uint32_t mask = (uint32_t)wmask;

  if (off >= MEM_SIZE) {
    printf("pmem_write: address 0x%08x out of range\n", addr);
    return;
  }

  if (mask & 0x1) m[off]   = (uint8_t)(data);
  if (mask & 0x2) m[off+1] = (uint8_t)(data >> 8);
  if (mask & 0x4) m[off+2] = (uint8_t)(data >> 16);
  if (mask & 0x8) m[off+3] = (uint8_t)(data >> 24);
}

extern "C" void npc_trap() {
  g_halted = true;
}
