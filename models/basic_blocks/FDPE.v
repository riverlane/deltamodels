
module FDPE(PRE, CE, D, C, Q);
  input PRE;
  input CE;
  input D;
  input C;
  output reg Q;
  parameter INIT = 1;

  always @(posedge C) begin
    Q <= D;
  end

endmodule
