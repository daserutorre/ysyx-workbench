#ifndef SEMU_H
#define SEMU_H

#include <stdint.h>

void ref_reset();
void ref_step();

uint8_t ref_get_pc();
uint8_t ref_get_reg(int i);  // i = 0..3

#endif // SEMU_H
