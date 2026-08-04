// A6 DUT — detect ramp crossing, drive loopback, observe D2A response
//
// When ramp_in goes high (analog crossed vth_hi), latch `crossed`
// and drive `ack_out` high for D2A feedback.
// ack_fb monitors the D2A response on the LOOPBACK node.

module a6_dut(
    input  ramp_in,
    input  ack_fb,
    output crossed,
    output ack_out
);
    reg cross_latch;
    reg ack_reg;
    reg fb_latch;

    initial begin
        cross_latch = 1'b0;
        ack_reg     = 1'b0;
        fb_latch    = 1'b0;
    end

    always @(posedge ramp_in) begin
        cross_latch <= 1'b1;
        ack_reg     <= 1'b1;
    end

    always @(posedge ack_fb) begin
        fb_latch <= 1'b1;
    end

    assign crossed = cross_latch;
    assign ack_out = ack_reg;
endmodule
