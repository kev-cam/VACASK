// A7 DUT — hierarchical D2A loopback
//
// When trig goes high, sets d2a_out to 1. Latches when fb (the D2A
// loopback readback, deep inside the analog hierarchy) transitions.

module a7_dut(
    input  trig,
    input  fb,
    output d2a_out,
    output fb_rose
);
    reg d2a_reg, fb_rose_r;

    initial begin
        d2a_reg   = 1'b0;
        fb_rose_r = 1'b0;
    end

    always @(posedge trig) d2a_reg   <= 1'b1;
    always @(posedge fb)   fb_rose_r <= 1'b1;

    assign d2a_out = d2a_reg;
    assign fb_rose = fb_rose_r;
endmodule
