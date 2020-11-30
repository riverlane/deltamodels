module ROM256X1 (O, A0, A1, A2, A3, A4, A5, A6, A7);
    parameter INIT = 256'h0000000000000000000000000000000000000000000000000000000000000000;
    output O;
    input  A0, A1, A2, A3, A4, A5, A6, A7;

    reg [255:0] mem;

    initial
	    mem = INIT;

    assign O = mem[{A7, A6, A5, A4, A3, A2, A1, A0}];

endmodule
