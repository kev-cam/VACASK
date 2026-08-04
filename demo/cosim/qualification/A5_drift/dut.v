// A5 DUT — clock edge detection for drift measurement
//
// Simple latch: goes high after first posedge of CLK.
// The actual edge counting is done by the check script from VCD data.

module a5_dut(
    input  clk,
    output counting
);
    reg cnt_active;

    initial cnt_active = 1'b0;

    always @(posedge clk) cnt_active <= 1'b1;

    assign counting = cnt_active;
endmodule
