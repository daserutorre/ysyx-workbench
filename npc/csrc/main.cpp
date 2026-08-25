#include "Vtop.h"
#include "verilated.h"
#include "verilated_fst_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <cassert>

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  Vtop *top = new Vtop;

  Verilated::traceEverOn(true);
  VerilatedFstC *tfp = new VerilatedFstC;
  top->trace(tfp, 5);
  tfp->open("wave.fst");

  vluint64_t main_time = 0;

  for (int i = 0; i < 20; i++) {
    int a = rand() & 1;
    int b = rand() & 1;
    top->a = a;
    top->b = b;
    top->eval();
    tfp->dump(main_time);
    main_time++;
    printf("a = %d, b = %d, f = %d\n", a, b, top->f);
    assert(top->f == (a ^ b));
  }

  tfp->close();
  delete top;
  return 0;
}
