#ifndef DPI_H
#define DPI_H

#include <stdint.h>

// Plain C++ helpers (not DPI-C themselves) used by the testbench.
void pmem_load_word(uint32_t addr, uint32_t word);
bool npc_is_halted();

// The actual DPI-C functions (pmem_read, pmem_write, npc_trap) are declared
// via "import DPI-C" in the Verilog, and Verilator auto-generates their
// prototypes. Our .cpp just needs to define them with matching extern "C"
// signatures -- no header declaration is required for that half.

#endif // DPI_H
