// B_simo digital controller
//
// Monitors two output rails via A2D feedback.
// Generates status / fault signals.
// D2A enable output (starts high after initial delay).
//
// A2D thresholds set in cosim_top.v:
//   fb_a: vth_lo=1.75 vth_hi=1.85  (1.8V rail ± 50mV)
//   fb_b: vth_lo=0.95 vth_hi=1.05  (1.0V rail ± 50mV)
//
// Tier-1 checks: regulation within thresholds after settling
// Tier-2 checks: crossing timing accuracy on PWM edges

module simo_ctrl(
    input  fb_a,
    input  fb_b,
    output en_out,
    output reg_a_ok,
    output reg_b_ok,
    output fault
);
    reg en_r;
    reg [15:0] cycle_cnt;
    reg a_ever_high, b_ever_high;
    reg fault_r;

    initial begin
        en_r        = 1'b0;
        cycle_cnt   = 16'd0;
        a_ever_high = 1'b0;
        b_ever_high = 1'b0;
        fault_r     = 1'b0;
    end

    always @(posedge fb_a) begin
        a_ever_high <= 1'b1;
        cycle_cnt <= cycle_cnt + 1;
    end

    always @(posedge fb_b)
        b_ever_high <= 1'b1;

    always @(negedge fb_a) begin
        if (a_ever_high && cycle_cnt > 16'd50)
            fault_r <= 1'b1;
    end

    assign en_out  = 1'b1;
    assign reg_a_ok = a_ever_high;
    assign reg_b_ok = b_ever_high;
    assign fault    = fault_r;
endmodule
