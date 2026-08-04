// A1 DUT — latches for ramp crossing events
//
// Inputs (A2D): ramp_slow, ramp_fast, rc_charge
// Outputs: slow_seen, fast_seen, rc_seen  (latch when posedge detected)

module a1_dut(
    input  ramp_slow,
    input  ramp_fast,
    input  rc_charge,
    output slow_seen,
    output fast_seen,
    output rc_seen
);
    reg slow_latch, fast_latch, rc_latch;

    initial begin
        slow_latch = 1'b0;
        fast_latch = 1'b0;
        rc_latch   = 1'b0;
    end

    always @(posedge ramp_slow) slow_latch <= 1'b1;
    always @(posedge ramp_fast) fast_latch <= 1'b1;
    always @(posedge rc_charge) rc_latch   <= 1'b1;

    assign slow_seen = slow_latch;
    assign fast_seen = fast_latch;
    assign rc_seen   = rc_latch;
endmodule
