#include "sEMU.h"

static uint8_t ref_PC = 0;
static uint8_t ref_R[4] = {0, 0, 0, 0};

static const uint8_t ref_M[16] = {
  0x8B, 0x91, 0xA0, 0xB1, 0x29, 0x17, 0xD1,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void ref_inst_cycle() {
  uint8_t inst = ref_M[ref_PC & 0xF];
  uint8_t op   = (inst >> 6) & 0x3;
  uint8_t rd   = (inst >> 4) & 0x3;
  uint8_t rs1  = (inst >> 2) & 0x3;
  uint8_t rs2  = inst & 0x3;
  uint8_t imm  = inst & 0xF;
  uint8_t addr = (inst >> 2) & 0xF;

  if (op == 0b00) {
    ref_R[rd] = (uint8_t)(ref_R[rs1] + ref_R[rs2]);
    ref_PC = (ref_PC + 1) & 0xF;
  } else if (op == 0b10) {
    ref_R[rd] = imm;
    ref_PC = (ref_PC + 1) & 0xF;
  } else if (op == 0b11) {
    if (ref_R[0] != ref_R[rs2]) ref_PC = addr;
    else ref_PC = (ref_PC + 1) & 0xF;
  }
}

void ref_reset() {
  ref_PC = 0;
  ref_R[0] = ref_R[1] = ref_R[2] = ref_R[3] = 0;
}

void ref_step() {
  ref_inst_cycle();
}

uint8_t ref_get_pc() {
  return ref_PC;
}

uint8_t ref_get_reg(int i) {
  return ref_R[i & 0x3];
}
