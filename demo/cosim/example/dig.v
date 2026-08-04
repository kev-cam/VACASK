module dig (input clk, output reg done);
    initial done = 1'b0;
    always @(posedge clk) done <= 1'b1;
endmodule
