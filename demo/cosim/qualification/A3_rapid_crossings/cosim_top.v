`timescale 1ns/1ps
module cosim_top;
    reg  narrow_30, narrow_10;
    wire pos30_seen, neg30_seen, pos10_seen, neg10_seen;

    a3_dut u_dut(
        .narrow_30(narrow_30), .narrow_10(narrow_10),
        .pos30_seen(pos30_seen), .neg30_seen(neg30_seen),
        .pos10_seen(pos10_seen), .neg10_seen(neg10_seen));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("NARROW_30", narrow_30, 0.6, 1.2);
        $cosim_a2d("NARROW_10", narrow_10, 0.6, 1.2);
        $cosim_run("analog.scs", 6e-6, 50e-9);
    end
endmodule
