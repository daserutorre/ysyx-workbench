module top (
  input         clk,
  input         rst,
  output [31:0] pc,
  input  [3:0]  dbg_addr,  // testbench-controlled: which register to inspect
  output [31:0] dbg_data,
  output [1:0]  dbg_state,
  output        dbg_ifu_reqValid,
  output        dbg_ifu_respValid,
  output        dbg_lsu_reqValid,
  output        dbg_lsu_respValid
);
  import "DPI-C" function void npc_trap();
  import "DPI-C" function void npc_commit();

  // ---- Overall control FSM ----
  //   IF_REQ  : issue an instruction-fetch request.
  //   IF_WAIT : wait for ifu_respValid. Once valid, decode this same
  //             cycle; if the instruction needs memory (load/store),
  //             *also* issue the LSU request this same cycle (address
  //             is already computable from decode) and move to
  //             MEM_WAIT. Otherwise commit right here and go back to
  //             IF_REQ.
  //   MEM_WAIT: wait for lsu_respValid. Once valid, commit and go back
  //             to IF_REQ.
  localparam IF_REQ = 2'd0, IF_WAIT = 2'd1, MEM_WAIT = 2'd2;
  reg [1:0] state;

  // ---- Fetch ----
  wire [31:0] inst;
  wire        ifu_respValid;
  wire        ifu_reqValid = (state == IF_REQ);
  wire [31:0] next_pc_wire;
  wire        commit;
  wire        pc_wen = commit;

  ifu u_ifu (
    .clk(clk), .rst(rst),
    .next_pc(next_pc_wire), .pc_wen(pc_wen),
    .ifu_reqValid(ifu_reqValid),
    .pc(pc), .inst(inst), .ifu_respValid(ifu_respValid)
  );

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

  wire needs_mem = is_lw | is_lbu | is_sw | is_sb;

  // ---- State transition ----
  always @(posedge clk) begin
    if (rst) state <= IF_REQ;
    else begin
      case (state)
        IF_REQ:   state <= IF_WAIT;
        IF_WAIT:  state <= !ifu_respValid ? IF_WAIT :
                            needs_mem     ? MEM_WAIT : IF_REQ;
        MEM_WAIT: state <= !lsu_respValid ? MEM_WAIT : IF_REQ;
        default:  state <= IF_REQ;
      endcase
    end
  end

  // ---- Commit: the cycle a real instruction's results are final ----
  assign commit = (state == IF_WAIT  && ifu_respValid && !needs_mem) ||
                  (state == MEM_WAIT && lsu_respValid);

  // ---- ebreak: notify the C++ testbench to stop the simulation ----
  always @(posedge clk) begin
    if (commit && is_ebreak) npc_trap();
  end

  // ---- Instruction retirement, for IPC measurement ----
  always @(posedge clk) begin
    if (!rst && commit) npc_commit();
  end

  // ---- Register file ----
  wire [31:0] rdata1, rdata2;
  wire [31:0] wdata;
  wire        wen = commit & (is_add | is_addi | is_lui | is_lw | is_lbu | is_jalr);

  RegisterFile #(4, 32) u_regfile (
    .clk(clk),
    .wdata(wdata), .waddr(rd[3:0]), .wen(wen),
    .raddr1(rs1[3:0]), .raddr2(rs2[3:0]),
    .rdata1(rdata1), .rdata2(rdata2),
    .raddr_dbg(dbg_addr), .rdata_dbg(dbg_data)
  );

  // ---- Execute: shared ALU ----
  reg [31:0] alu_b;
  always @(*) begin
    if (is_add) alu_b = rdata2;
    else if (is_sw) alu_b = imm_s;
    else if (is_sb) alu_b = imm_s;
    else alu_b = imm_i;
  end

  wire [31:0] alu_result;
  alu u_alu (.a(rdata1), .b(alu_b), .c(alu_result));

  wire [31:0] word_addr = {alu_result[31:2], 2'b00};
  wire [1:0]  byte_lane = alu_result[1:0];

  // ---- LSU bus wiring: request issued the SAME cycle decode determines
  // memory access is needed (overlapping with IF_WAIT), not a separate
  // dedicated cycle. ----
  wire        lsu_reqValid = (state == IF_WAIT) && ifu_respValid && needs_mem;
  wire        lsu_wen      = is_sw | is_sb;
  wire [31:0] sb_shifted_data = ({24'b0, rdata2[7:0]}) << (byte_lane * 8);
  wire [3:0]  sb_mask         = 4'b0001 << byte_lane;
  wire [31:0] lsu_wdata = is_sw ? rdata2 : sb_shifted_data;
  wire [3:0]  lsu_wmask = is_sw ? 4'b1111 : sb_mask;
  wire        lsu_respValid;
  wire [31:0] lsu_rdata;

  lsu u_lsu (
    .clk(clk), .rst(rst),
    .lsu_reqValid(lsu_reqValid),
    .lsu_addr(word_addr),
    .lsu_wen(lsu_wen),
    .lsu_wdata(lsu_wdata),
    .lsu_wmask(lsu_wmask),
    .lsu_respValid(lsu_respValid),
    .lsu_rdata(lsu_rdata)
  );

  wire [31:0] load_word = lsu_rdata;
  wire [31:0] lbu_result = (load_word >> (byte_lane * 8)) & 32'hFF;

  // ---- Execute: jump target and link ----
  wire [31:0] jalr_target = alu_result & ~32'h1;
  wire [31:0] jalr_link   = pc + 4;

  // ---- Writeback mux ----
  assign wdata = ({32{is_add | is_addi}} & alu_result) |
                 ({32{is_lui}}           & imm_u)       |
                 ({32{is_lw}}            & load_word)   |
                 ({32{is_lbu}}           & lbu_result)  |
                 ({32{is_jalr}}          & jalr_link);

  // ---- Next PC (sampled by ifu's PC register only when pc_wen/commit) ----
  reg [31:0] next_pc;
  always @(*) begin
    if (is_ebreak) next_pc = pc;
    else if (is_jalr) next_pc = jalr_target;
    else next_pc = pc + 4;
  end
  assign next_pc_wire = next_pc;

  assign dbg_state = state;
  assign dbg_ifu_reqValid = ifu_reqValid;
  assign dbg_ifu_respValid = ifu_respValid;
  assign dbg_lsu_reqValid = lsu_reqValid;
  assign dbg_lsu_respValid = lsu_respValid;
endmodule
