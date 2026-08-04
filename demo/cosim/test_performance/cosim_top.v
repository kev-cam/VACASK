`timescale 1ns/1ps
module cosim_top;
    reg  clk, rst_n, narrow_pulse;
    wire reached_10, reached_25, reached_50, pulse_seen;

    perf_dut u_dut(
        .clk(clk), .rst_n(rst_n), .narrow_pulse(narrow_pulse),
        .reached_10(reached_10), .reached_25(reached_25),
        .reached_50(reached_50), .pulse_seen(pulse_seen));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("CLK",    clk,          0.6, 1.2);
        $cosim_a2d("RST_N",  rst_n,        0.6, 1.2);
        $cosim_a2d("NARROW", narrow_pulse, 0.6, 1.2);
        $cosim_run("analog.scs", 60e-6, 100e-9);
    end
endmodule
