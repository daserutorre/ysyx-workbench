module regfile (
  input        clk,
  input        rst,
  input  [1:0] waddr,
  input  [7:0] wdata,
  input        wen,
  input  [1:0] raddr1,
  input  [1:0] raddr2,
  output [7:0] rdata1,
  output [7:0] rdata2,
  output [7:0] r0_data,
  input  [1:0] raddr_dbg,
  output [7:0] rdata_dbg
);
  wire [7:0] r0, r1, r2, r3;

  Reg #(8, 8'h0) reg0 (clk, rst, wdata, r0, wen & (waddr == 2'd0));
  Reg #(8, 8'h0) reg1 (clk, rst, wdata, r1, wen & (waddr == 2'd1));
  Reg #(8, 8'h0) reg2 (clk, rst, wdata, r2, wen & (waddr == 2'd2));
  Reg #(8, 8'h0) reg3 (clk, rst, wdata, r3, wen & (waddr == 2'd3));

  assign r0_data = r0;

  MuxKey #(4, 2, 8) rmux1 (rdata1, raddr1, {
    2'd0, r0,
    2'd1, r1,
    2'd2, r2,
    2'd3, r3
  });

  MuxKey #(4, 2, 8) rmux2 (rdata2, raddr2, {
    2'd0, r0,
    2'd1, r1,
    2'd2, r2,
    2'd3, r3
  });

  MuxKey #(4, 2, 8) rmux_dbg (rdata_dbg, raddr_dbg, {
    2'd0, r0,
    2'd1, r1,
    2'd2, r2,
    2'd3, r3
  });
endmodule
