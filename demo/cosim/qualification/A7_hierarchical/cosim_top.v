`timescale 1ns/1ps
module cosim_top;
    reg  trig, fb;
    wire d2a_out, fb_rose;

    a7_dut u_dut(.trig(trig), .fb(fb), .d2a_out(d2a_out), .fb_rose(fb_rose));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("xcore:xana:TRIG", trig,    0.6, 1.2);
        $cosim_a2d("xcore:xana:FB",   fb,      0.6, 1.2);
        $cosim_d2a("xcore:xana:FB",   d2a_out, 1.8, 1000, 1e-12);
        $cosim_run("analog.scs", 5e-6, 50e-9);
    end
endmodule
