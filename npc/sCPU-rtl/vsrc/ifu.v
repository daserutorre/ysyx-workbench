module ifu (
  input  [3:0] addr,
  output [7:0] inst
);
  MuxKeyWithDefault #(7, 4, 8) i0 (
    inst, addr, 8'h00, {
      4'h0, 8'h8B,  // li r0, 11
      4'h1, 8'h91,  // li r1, 1
      4'h2, 8'hA0,  // li r2, 0
      4'h3, 8'hB1,  // li r3, 1
      4'h4, 8'h29,  // add r2, r2, r1
      4'h5, 8'h17,  // add r1, r1, r3
      4'h6, 8'hD1   // bner0 4, r1
    }
  );
endmodule
