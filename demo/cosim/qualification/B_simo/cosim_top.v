`timescale 1ns/1ps
module cosim_top;
    reg  fb_a, fb_b;
    wire en_out, reg_a_ok, reg_b_ok, fault;

    simo_ctrl u_dut(
        .fb_a(fb_a), .fb_b(fb_b), .en_out(en_out),
        .reg_a_ok(reg_a_ok), .reg_b_ok(reg_b_ok), .fault(fault));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("OUTA", fb_a, 1.75, 1.85);
        $cosim_a2d("OUTB", fb_b, 0.95, 1.05);
        $cosim_d2a("EN_NODE", en_out, 1.8, 1000, 1e-12);
        $cosim_run("analog.scs", 200e-6, 50e-9);
    end
endmodule
