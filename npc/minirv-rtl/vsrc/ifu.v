module ifu (
  input         clk,
  input         rst,
  input  [31:0] next_pc,
  output [31:0] pc,
  output [31:0] inst
);
  import "DPI-C" function int pmem_read(input int raddr);

  Reg #(32, 32'h80000000) u_pc (.clk(clk), .rst(rst), .din(next_pc), .dout(pc), .wen(1'b1));

  assign inst = pmem_read(pc);
endmodule
