`timescale 1ns/1ps
module cosim_top;
    reg  ramp_in, ack_fb;
    wire crossed, ack_out;

    a6_dut u_dut(
        .ramp_in(ramp_in), .ack_fb(ack_fb),
        .crossed(crossed), .ack_out(ack_out));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("RAMP",     ramp_in, 0.5,  1.55);
        $cosim_a2d("LOOPBACK", ack_fb,  0.5,  1.5);
        $cosim_d2a("LOOPBACK", ack_out, 3.0, 1e3, 1e-12);
        $cosim_run("analog.scs", 10e-6, 500e-9);
    end
endmodule
