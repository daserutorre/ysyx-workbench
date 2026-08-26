module top (
  input         clk,
  input         rst,
  output [31:0] pc,
  input  [3:0]  dbg_addr,  // testbench-controlled: which register to inspect
  output [31:0] dbg_data
);
  import "DPI-C" function int  pmem_read(input int raddr);
  import "DPI-C" function void pmem_write(input int waddr, input int wdata, input int wmask);
  import "DPI-C" function void npc_trap();

  // ---- Fetch ----
  wire [31:0] next_pc_wire;  // input
  wire [31:0] inst;          // output

  ifu u_ifu (.clk(clk), .rst(rst), .next_pc(next_pc_wire), .pc(pc), .inst(inst));

  // ---- Decode ----
  wire [6:0]  opcode, funct7;
  wire [4:0]  rd, rs1, rs2;
  wire [2:0]  funct3;
  wire [31:0] imm_i, imm_s, imm_u;
  wire        is_add, is_addi, is_lui, is_lw, is_lbu, is_sw, is_sb, is_jalr, is_ebreak;

  decoder u_decoder (
    .inst(inst),
    .opcode(opcode), .rd(rd), .funct3(funct3),
    .rs1(rs1), .rs2(rs2), .funct7(funct7),
    .imm_i(imm_i), .imm_s(imm_s), .imm_u(imm_u),
    .is_add(is_add), .is_addi(is_addi), .is_lui(is_lui),
    .is_lw(is_lw), .is_lbu(is_lbu), .is_sw(is_sw), .is_sb(is_sb),
    .is_jalr(is_jalr), .is_ebreak(is_ebreak)
  );

  // ---- ebreak: notify the C++ testbench to stop the simulation ----
  always @(posedge clk) begin
    if (is_ebreak) npc_trap();
  end

  // ---- Register file ----
  wire [31:0] rdata1, rdata2;   // output
  wire [31:0] wdata;            // input
  wire        wen = is_add | is_addi | is_lui | is_lw | is_lbu | is_jalr;

  RegisterFile #(4, 32) u_regfile (
    .clk(clk),
    .wdata(wdata), .waddr(rd[3:0]), .wen(wen),
    .raddr1(rs1[3:0]), .raddr2(rs2[3:0]),
    .rdata1(rdata1), .rdata2(rdata2),
    .raddr_dbg(dbg_addr), .rdata_dbg(dbg_data)
  );

  // ---- Execute: shared ALU (one adder, muxed second operand) ----
  reg [31:0] alu_b;
  always @(*) begin
    if (is_add) alu_b = rdata2;
    else if (is_sw) alu_b = imm_s;
    else if (is_sb) alu_b = imm_s;
    else alu_b = imm_i;
  end

  wire [31:0] alu_result;
  alu u_alu (.a(rdata1), .b(alu_b), .c(alu_result));

  // alu_result serves double duty depending on the instruction:
  //   add/addi -> the arithmetic result itself
  //   lw/lbu   -> the load address (rdata1 + imm_i)
  //   sw/sb    -> the store address (rdata1 + imm_s)
  //   jalr     -> the jump target base (rdata1 + imm_i), before masking bit 0
  wire [31:0] word_addr = {alu_result[31:2], 2'b00};
  wire [1:0]  byte_lane = alu_result[1:0];

  // ---- Execute: reads (lw/lbu) ----
  wire [31:0] load_word = pmem_read(word_addr);
  wire [31:0] lbu_result = (load_word >> (byte_lane * 8)) & 32'hFF;

  // ---- Execute: writes (sw/sb), triggered once per cycle on the clock edge ----
  wire [31:0] sb_shifted_data = ({24'b0, rdata2[7:0]}) << (byte_lane * 8);
  wire [3:0]  sb_mask = 4'b0001 << byte_lane;

  always @(posedge clk) begin
    if (is_sw) pmem_write(word_addr, rdata2, {28'b0, 4'b1111});
    else if (is_sb) pmem_write(word_addr, sb_shifted_data, {28'b0, sb_mask});
  end

  // ---- Execute: jump target and link ----
  wire [31:0] jalr_target = alu_result & ~32'h1;
  wire [31:0] jalr_link   = pc + 4;

  // ---- Writeback mux ----
  assign wdata = ({32{is_add | is_addi}} & alu_result) |
                 ({32{is_lui}}           & imm_u)       |
                 ({32{is_lw}}            & load_word)   |
                 ({32{is_lbu}}           & lbu_result)  |
                 ({32{is_jalr}}          & jalr_link);

  // ---- Update PC ----
  reg [31:0] next_pc;
  always @(*) begin
    if (is_ebreak) next_pc = pc;         // freeze PC on trap
    else if (is_jalr) next_pc = jalr_target;
    else next_pc = pc + 4;
  end
  assign next_pc_wire = next_pc;
endmodule
