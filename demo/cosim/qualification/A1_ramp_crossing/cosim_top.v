`timescale 1ns/1ps
module cosim_top;
    reg  ramp_slow, ramp_fast, rc_charge;
    wire slow_seen, fast_seen, rc_seen;

    a1_dut u_dut(
        .ramp_slow(ramp_slow), .ramp_fast(ramp_fast), .rc_charge(rc_charge),
        .slow_seen(slow_seen), .fast_seen(fast_seen), .rc_seen(rc_seen));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("RAMP_SLOW", ramp_slow, 0.6, 1.2);
        $cosim_a2d("RAMP_FAST", ramp_fast, 0.6, 1.2);
        $cosim_a2d("RC_CHARGE", rc_charge, 0.6, 1.2);
        $cosim_run("analog.scs", 12e-6, 50e-9);
    end
endmodule
