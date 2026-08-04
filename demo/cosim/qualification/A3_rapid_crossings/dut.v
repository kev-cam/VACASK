// A3 DUT — rapid crossing event detection
//
// Latches first posedge and negedge on each narrow pulse signal.
// Expected: exactly 1 posedge + 1 negedge per pulse.

module a3_dut(
    input  narrow_30,
    input  narrow_10,
    output pos30_seen,
    output neg30_seen,
    output pos10_seen,
    output neg10_seen
);
    reg p30, n30, p10, n10;

    initial begin
        p30 = 1'b0;
        n30 = 1'b0;
        p10 = 1'b0;
        n10 = 1'b0;
    end

    always @(posedge narrow_30) p30 <= 1'b1;
    always @(negedge narrow_30) n30 <= 1'b1;
    always @(posedge narrow_10) p10 <= 1'b1;
    always @(negedge narrow_10) n10 <= 1'b1;

    assign pos30_seen = p30;
    assign neg30_seen = n30;
    assign pos10_seen = p10;
    assign neg10_seen = n10;
endmodule
