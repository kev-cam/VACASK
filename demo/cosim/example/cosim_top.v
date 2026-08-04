`timescale 1ns / 1ps
module cosim_top;
    reg  clk_v1;
    wire q_v1;
    wire done_dig;

    verilog1 u_v1  (.clk(clk_v1), .q(q_v1));
    dig      u_dig (.clk(q_v1),   .done(done_dig));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("xcore:xana:CLK", clk_v1, 0.6, 1.2);
        $cosim_d2a("xcore:xana:Q",   q_v1,   1.8, 1e3, 1e-12);
        $cosim_run("analog.scs", 2001e-6, 50e-9);
    end
endmodule
