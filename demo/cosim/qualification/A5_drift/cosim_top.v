`timescale 1ns/1ps
module cosim_top;
    reg  clk;
    wire counting;

    a5_dut u_dut(.clk(clk), .counting(counting));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("CLK", clk, 0.6, 1.2);
        $cosim_run("analog.scs", 2001e-6, 50e-9);
    end
endmodule
