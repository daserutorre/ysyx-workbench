// Simple 8-bit Fibonacci LFSR, used to generate a pseudo-random delay
// value each time a new memory request starts. Taps chosen for a
// maximal-length 8-bit sequence (polynomial x^8+x^6+x^5+x^4+1).
module lfsr8 (
  input        clk,
  input        rst,
  input        advance,   // pull a new random value (shift the LFSR) this cycle
  output [7:0] value
);
  reg [7:0] state;

  wire feedback = state[7] ^ state[5] ^ state[4] ^ state[3];

  always @(posedge clk) begin
    if (rst) state <= 8'hA5; // any nonzero seed
    else if (advance) state <= {state[6:0], feedback};
  end

  assign value = state;
endmodule
