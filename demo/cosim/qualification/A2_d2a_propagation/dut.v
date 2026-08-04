// A2 DUT — D2A loopback test
//
// When trig_on goes high (at ~1µs), sets d2a_out to 1.
// When trig_off goes high (at ~3µs), sets d2a_out to 0.
// Latches when d2a_fb (loopback readback) transitions.

module a2_dut(
    input  trig_on,
    input  trig_off,
    input  d2a_fb,
    output d2a_out,
    output fb_rose,
    output fb_fell
);
    reg d2a_reg;
    reg fb_rose_r, fb_fell_r;

    initial begin
        d2a_reg  = 1'b0;
        fb_rose_r = 1'b0;
        fb_fell_r = 1'b0;
    end

    always @(posedge trig_on)  d2a_reg <= 1'b1;
    always @(posedge trig_off) d2a_reg <= 1'b0;

    always @(posedge d2a_fb) fb_rose_r <= 1'b1;
    always @(negedge d2a_fb) fb_fell_r <= 1'b1;

    assign d2a_out = d2a_reg;
    assign fb_rose = fb_rose_r;
    assign fb_fell = fb_fell_r;
endmodule
