`timescale 1ns / 1ps

module perf_dut (
    input  clk,
    input  rst_n,
    input  narrow_pulse,
    output reached_10,
    output reached_25,
    output reached_50,
    output pulse_seen
);

    reg [7:0] count;
    assign reached_10 = (count >= 8'd10);
    assign reached_25 = (count >= 8'd25);
    assign reached_50 = (count >= 8'd50);

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            count <= 8'd0;
        else
            count <= count + 8'd1;
    end

    reg pulse_latch;
    assign pulse_seen = pulse_latch;

    always @(posedge narrow_pulse or negedge rst_n) begin
        if (!rst_n)
            pulse_latch <= 1'b0;
        else
            pulse_latch <= 1'b1;
    end

endmodule
