// A4 DUT — multi-rail simultaneous crossing detection
//
// Latches when each rail goes high. Also records which went first.

module a4_dut(
    input  rail_a,
    input  rail_b,
    output a_seen,
    output b_seen,
    output a_first
);
    reg a_latch, b_latch;
    reg a_first_r;
    reg either_seen;

    initial begin
        a_latch    = 1'b0;
        b_latch    = 1'b0;
        a_first_r  = 1'b0;
        either_seen = 1'b0;
    end

    always @(posedge rail_a) begin
        a_latch <= 1'b1;
        if (!either_seen) begin
            a_first_r  <= 1'b1;
            either_seen <= 1'b1;
        end
    end

    always @(posedge rail_b) begin
        b_latch <= 1'b1;
        if (!either_seen)
            either_seen <= 1'b1;
    end

    assign a_seen  = a_latch;
    assign b_seen  = b_latch;
    assign a_first = a_first_r;
endmodule
