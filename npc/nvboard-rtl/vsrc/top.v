module top(
  input clk,
  input rst,
  output [15:0] led
);
  reg [31:0] count;
  reg [15:0] led_r;

  always @(posedge clk) begin
    if (rst) begin led_r <= 1; count <= 0; end
    else begin
      if (count == 0) led_r <= {led_r[14:0], led_r[15]};
      count <= (count >= 5000000 ? 32'b0 : count + 1);
    end
  end

  assign led = led_r;
endmodule
