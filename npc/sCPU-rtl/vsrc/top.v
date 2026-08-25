module top (
  input        clk,
  input        rst,
  output [3:0] pc_out,
  output [7:0] sum
);
  wire [3:0] pc;
  wire [3:0] next_pc;
  wire [7:0] inst;

  wire [1:0] op   = inst[7:6];
  wire [1:0] rd   = inst[5:4];
  wire [1:0] rs1  = inst[3:2];
  wire [1:0] rs2  = inst[1:0];
  wire [3:0] imm  = inst[3:0];
  wire [3:0] baddr = inst[5:2];

  wire is_add   = (op == 2'b00);
  wire is_li    = (op == 2'b10);
  wire is_bner0 = (op == 2'b11);

  wire [7:0] rdata1, rdata2, r0_data;
  wire [7:0] alu_result = rdata1 + rdata2;
  wire [7:0] imm_ext = {4'b0, imm};
  wire [7:0] wdata = ({8{is_add}} & alu_result) | ({8{is_li}} & imm_ext);
  wire       wen = is_add | is_li;

  wire branch_taken = is_bner0 & (r0_data != rdata2);
  assign next_pc = branch_taken ? baddr : (pc + 4'b1);

  Reg #(4, 4'h0) u_pc (clk, rst, next_pc, pc, 1'b1);

  ifu u_ifu (pc, inst);

  regfile u_regfile (
    .clk(clk), .rst(rst),
    .waddr(rd), .wdata(wdata), .wen(wen),
    .raddr1(rs1), .raddr2(rs2),
    .rdata1(rdata1), .rdata2(rdata2),
    .r0_data(r0_data),
    .raddr_dbg(2'd2), .rdata_dbg(sum)
  );

  assign pc_out = pc;
endmodule
