module RegisterFile #(ADDR_WIDTH = 4, DATA_WIDTH = 32) (
  input clk,
  input [DATA_WIDTH-1:0] wdata,
  input [ADDR_WIDTH-1:0] waddr,
  input [ADDR_WIDTH-1:0] raddr1,
  input [ADDR_WIDTH-1:0] raddr2,
  input wen,
  output reg [DATA_WIDTH-1:0] rdata1,
  output reg [DATA_WIDTH-1:0] rdata2,
  input [ADDR_WIDTH-1:0] raddr_dbg,
  output reg [DATA_WIDTH-1:0] rdata_dbg
);
  reg [DATA_WIDTH-1:0] rf [2**ADDR_WIDTH-1:0];
  always @(posedge clk) begin
    if (wen) rf[waddr] <= wdata;
  end

  // x0 is hardwired to zero: reads of register 0 always return 0,
  // regardless of what (if anything) was ever written to rf[0].
  always @(*) begin
    if (raddr1 == 0) rdata1 = {DATA_WIDTH{1'b0}};
    else rdata1 = rf[raddr1];
  end

  always @(*) begin
    if (raddr2 == 0) rdata2 = {DATA_WIDTH{1'b0}};
    else rdata2 = rf[raddr2];
  end

  always @(*) begin
    if (raddr_dbg == 0) rdata_dbg = {DATA_WIDTH{1'b0}};
    else rdata_dbg = rf[raddr_dbg];
  end
endmodule
