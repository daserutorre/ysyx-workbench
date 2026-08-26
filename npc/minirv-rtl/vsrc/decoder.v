module decoder (
  input  [31:0] inst,
  output [6:0]  opcode,
  output [4:0]  rd,
  output [2:0]  funct3,
  output [4:0]  rs1,
  output [4:0]  rs2,
  output [6:0]  funct7,
  output [31:0] imm_i,
  output [31:0] imm_s,
  output [31:0] imm_u,
  output        is_add,
  output        is_addi,
  output        is_lui,
  output        is_lw,
  output        is_lbu,
  output        is_sw,
  output        is_sb,
  output        is_jalr,
  output        is_ebreak
);
  assign opcode = inst[6:0];
  assign rd     = inst[11:7];
  assign funct3 = inst[14:12];
  assign rs1    = inst[19:15];
  assign rs2    = inst[24:20];
  assign funct7 = inst[31:25];

  // I-type immediate: inst[31:20], sign-extended.
  assign imm_i = {{20{inst[31]}}, inst[31:20]};

  // S-type immediate: inst[31:25] | inst[11:7], sign-extended.
  assign imm_s = {{20{inst[31]}}, inst[31:25], inst[11:7]};

  // U-type immediate: inst[31:12] << 12 (already in upper-bit position).
  assign imm_u = {inst[31:12], 12'b0};

  assign is_add    = (opcode == 7'b0110011) && (funct3 == 3'b000) && (funct7 == 7'b0000000);
  assign is_addi   = (opcode == 7'b0010011) && (funct3 == 3'b000);
  assign is_lui    = (opcode == 7'b0110111);
  assign is_lw     = (opcode == 7'b0000011) && (funct3 == 3'b010);
  assign is_lbu    = (opcode == 7'b0000011) && (funct3 == 3'b100);
  assign is_sw     = (opcode == 7'b0100011) && (funct3 == 3'b010);
  assign is_sb     = (opcode == 7'b0100011) && (funct3 == 3'b000);
  assign is_jalr   = (opcode == 7'b1100111) && (funct3 == 3'b000);
  assign is_ebreak = (inst == 32'h00100073);
endmodule
