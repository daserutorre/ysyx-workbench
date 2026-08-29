module lsu (
  input         clk,
  input         rst,
  input         lsu_reqValid,
  input  [31:0] lsu_addr,
  input         lsu_wen,
  input  [31:0] lsu_wdata,
  input  [ 3:0] lsu_wmask,
  output        lsu_respValid,
  output [31:0] lsu_rdata
);
  import "DPI-C" function int  pmem_read(input int raddr);
  import "DPI-C" function void pmem_write(input int waddr, input int wdata, input int wmask);

  // ---- Random per-request delay, via LFSR. Masked down to a small
  // range (1-16 cycles) so tests still finish in reasonable time. ----
  wire [7:0] lfsr_val;
  wire       start_new_req = lsu_reqValid && !active;
  lfsr8 u_lfsr (.clk(clk), .rst(rst), .advance(start_new_req), .value(lfsr_val));
  wire [31:0] rand_delay = {28'b0, (lfsr_val[3:0] | 4'h1)}; // 1-15 cycles, never 0

  reg [31:0] rdata_r;
  reg        respValid_r;
  reg        active;
  reg [31:0] cnt;

  always @(posedge clk) begin
    if (rst) begin
      rdata_r     <= 32'b0;
      respValid_r <= 1'b0;
      active      <= 1'b0;
      cnt         <= 32'b0;
    end else begin
      respValid_r <= 1'b0;
      if (lsu_reqValid && !active) begin
        if (rand_delay <= 1) begin
          rdata_r <= (!lsu_wen) ? pmem_read(lsu_addr) : 32'b0;
          if (lsu_wen) begin
            pmem_write(lsu_addr, lsu_wdata, {28'b0, lsu_wmask});
          end
          respValid_r <= 1'b1;
        end else begin
          active <= 1'b1;
          cnt    <= rand_delay - 1;
        end
      end else if (active) begin
        if (cnt == 1) begin
          rdata_r <= (!lsu_wen) ? pmem_read(lsu_addr) : 32'b0;
          if (lsu_wen) begin
            pmem_write(lsu_addr, lsu_wdata, {28'b0, lsu_wmask});
          end
          respValid_r <= 1'b1;
          active      <= 1'b0;
        end else begin
          cnt <= cnt - 1;
        end
      end
    end
  end

  assign lsu_rdata = rdata_r;
  assign lsu_respValid = respValid_r;
endmodule
