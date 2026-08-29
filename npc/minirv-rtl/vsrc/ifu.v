module ifu (
  input         clk,
  input         rst,
  input  [31:0] next_pc,
  input         pc_wen,        // update PC (asserted on commit of the previous instruction)
  input         ifu_reqValid,  // top.v asserts this to issue a fetch request this cycle
  output [31:0] pc,
  output [31:0] inst,
  output        ifu_respValid  // 1 the cycle `inst` holds a genuinely fresh, valid instruction
);
  import "DPI-C" function int pmem_read(input int raddr);

  Reg #(32, 32'h80000000) u_pc (.clk(clk), .rst(rst), .din(next_pc), .dout(pc), .wen(pc_wen));

  reg [31:0] rdata_r;
  reg        respValid_r;

  always @(posedge clk) begin
    if (rst) begin
      rdata_r     <= 32'b0;
      respValid_r <= 1'b0;
    end else begin
      rdata_r     <= ifu_reqValid ? pmem_read(pc) : rdata_r;
      respValid_r <= ifu_reqValid;
    end
  end

  assign inst = rdata_r;
  assign ifu_respValid = respValid_r;
endmodule
