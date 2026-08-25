#ifndef MINIRVEMU_H
#define MINIRVEMU_H

#include <stdint.h>

void emu_reset();
void emu_step();

uint32_t emu_get_pc();
uint32_t emu_get_reg(int i);  // i = 0..15 (RV32E: 16 GPRs)

// Load a 32-bit instruction word into memory at byte address addr.
void emu_load_word(uint32_t addr, uint32_t word);

// True once the program has executed an ebreak instruction.
int emu_halted();

#endif // MINIRVEMU_H
