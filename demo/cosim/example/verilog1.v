module verilog1 (input clk, output q);
    reg r; initial r = 1'b0;
    always @(posedge clk) r <= 1'b1;
    assign q = r;
endmodule
