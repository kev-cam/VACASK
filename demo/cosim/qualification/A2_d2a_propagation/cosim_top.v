`timescale 1ns/1ps
module cosim_top;
    reg  trig_on, trig_off, d2a_fb;
    wire d2a_out, fb_rose, fb_fell;

    a2_dut u_dut(
        .trig_on(trig_on), .trig_off(trig_off), .d2a_fb(d2a_fb),
        .d2a_out(d2a_out), .fb_rose(fb_rose), .fb_fell(fb_fell));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("TRIG_ON",  trig_on,  0.6, 1.2);
        $cosim_a2d("TRIG_OFF", trig_off, 0.6, 1.2);
        $cosim_a2d("D2A_NODE", d2a_fb,   0.6, 1.2);
        $cosim_d2a("D2A_NODE", d2a_out,  1.8, 1000, 1e-12);
        $cosim_run("analog.scs", 5e-6, 50e-9);
    end
endmodule
