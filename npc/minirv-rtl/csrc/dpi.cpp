#include "dpi.h"
#include <string.h>
#include <stdio.h>

#define MEM_BASE     0x80000000u
#define MEM_SIZE     (128 * 1024 * 1024) // 128MB
#define UART_ADDR    0x10000000u
#define TIMER_LO_ADDR 0x20000000u
#define TIMER_HI_ADDR 0x20000004u

// Assumed clock frequency for the *simulated* CPU, used to convert a cycle
// count into a "simulated microseconds" value for the timer registers.
#define CLOCK_FREQ_HZ 100000000ull // 100MHz

static uint8_t *pmem = nullptr;
static bool g_halted = false;
static uint64_t g_cycle_count = 0;
static uint64_t g_instret_count = 0;

static uint8_t *get_pmem() {
  if (!pmem) {
    pmem = new uint8_t[MEM_SIZE];
    memset(pmem, 0, MEM_SIZE);
  }
  return pmem;
}

static uint64_t get_uptime_us() {
  return (g_cycle_count * 1000000ull) / CLOCK_FREQ_HZ;
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

void npc_tick() {
  g_cycle_count++;
}

uint64_t npc_get_cycle_count() {
  return g_cycle_count;
}

uint64_t npc_get_instret_count() {
  return g_instret_count;
}

// ---- DPI-C functions (RTL-facing, called directly from Verilog) ----

extern "C" int pmem_read(int raddr) {
  uint32_t addr = ((uint32_t)raddr) & ~0x3u; // force word alignment

  if (addr == TIMER_LO_ADDR) {
    return (int)(uint32_t)(get_uptime_us() & 0xFFFFFFFFu);
  }
  if (addr == TIMER_HI_ADDR) {
    return (int)(uint32_t)(get_uptime_us() >> 32);
  }

  uint8_t *m = get_pmem();
  uint32_t off = addr - MEM_BASE;
  if (off >= MEM_SIZE) {
    return 0;
  }
  return (int)((uint32_t)m[off]        |
               (uint32_t)m[off+1] << 8  |
               (uint32_t)m[off+2] << 16 |
               (uint32_t)m[off+3] << 24);
}

extern "C" void pmem_write(int waddr, int wdata, int wmask) {
  uint32_t addr = ((uint32_t)waddr) & ~0x3u;
  uint32_t data = (uint32_t)wdata;
  uint32_t mask = (uint32_t)wmask;

  if (addr == UART_ADDR) {
    if (mask & 0x1) putchar((char)(data & 0xFF));
    fflush(stdout);
    return;
  }

  uint8_t *m = get_pmem();
  uint32_t off = addr - MEM_BASE;

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

// Called by the RTL exactly once per instruction that genuinely completes
// (not once per cycle) -- lets us measure real IPC once IFU has idle/wait
// cycles where no instruction retires.
extern "C" void npc_commit() {
  g_instret_count++;
}
