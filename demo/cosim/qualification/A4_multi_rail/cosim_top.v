`timescale 1ns/1ps
module cosim_top;
    reg  rail_a, rail_b;
    wire a_seen, b_seen, a_first;

    a4_dut u_dut(
        .rail_a(rail_a), .rail_b(rail_b),
        .a_seen(a_seen), .b_seen(b_seen), .a_first(a_first));

    initial begin
        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);
        $cosim_a2d("RAIL_A", rail_a, 0.6, 1.2);
        $cosim_a2d("RAIL_B", rail_b, 0.6, 1.2);
        $cosim_run("analog.scs", 4e-6, 50e-9);
    end
endmodule
