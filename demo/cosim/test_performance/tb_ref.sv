`timescale 1ns / 1ps

module tb_ref;

// Analog clock: pulse val0=0 val1=1.8V, delay=5us, rise=10ns, fall=10ns
// width=490ns, period=1us.  E2L vthi=1.2V, vtlo=0.6V.
// Rising edge vthi crossing: delay + (1.2/1.8)*10ns = 5000 + 6.667 ~ 5007ns
// Falling edge vtlo crossing: delay + 10 + 490 + (1-0.6/1.8)*10 = 5503.333 ~ 5503ns
// Digital period = 1000ns, first posedge at 5007ns.
// At 1ns resolution: posedge at 5007, 6007, 7007, ...
reg clk;
initial begin
    clk = 1'b0;
    #(5007);
    forever begin
        clk = 1'b1;
        #(497);      // high for ~497ns (rise-adjusted half-period)
        clk = 1'b0;
        #(503);      // low for ~503ns
    end
end

// Reset: PWL starts at 1.8V, falls at 1us (10ns ramp), rises at 4us (10ns ramp)
// vtlo crossing on fall: 1e4 + (1.8-0.6)/1.8 * 10 = 10006.67 ~ 10007ns
// vthi crossing on rise: 4e4 + (1.2/1.8) * 10 = 40006.67 ~ 40007ns
// Hmm wait, 1us = 1000ns, 4us = 4000ns with 10ns ramps.
// Fall: 1000 + (1-0.6/1.8)*10 = 1000 + 6.67 = 1007ns → rst_n goes low
// Rise: 4000 + (1.2/1.8)*10 = 4000 + 6.67 = 4007ns → rst_n goes high
reg rst_n;
initial begin
    rst_n = 1'b1;
    #(1007);
    rst_n = 1'b0;
    #(3000);       // 4007ns
    rst_n = 1'b1;
end

// Narrow pulse at 30us: 50ns wide, 1ns rise/fall
// vthi crossing on rise: 30000 + (1.2/1.8)*1 = 30001ns
// vtlo crossing on fall: 30000 + 1 + 50 + (1-0.6/1.8)*1 = 30051.67 ~ 30052ns
reg narrow_pulse;
initial begin
    narrow_pulse = 1'b0;
    #(30001);
    narrow_pulse = 1'b1;
    #(51);
    narrow_pulse = 1'b0;
end

wire reached_10, reached_25, reached_50, pulse_seen;

perf_dut dut (
    .clk           (clk),
    .rst_n         (rst_n),
    .narrow_pulse  (narrow_pulse),
    .reached_10    (reached_10),
    .reached_25    (reached_25),
    .reached_50    (reached_50),
    .pulse_seen    (pulse_seen)
);

always @(posedge clk)
    $display("[%0t] count=%0d", $realtime, dut.count);

always @(reached_10)
    if (reached_10) $display("[%0t] *** reached_10 ***", $realtime);
always @(reached_25)
    if (reached_25) $display("[%0t] *** reached_25 ***", $realtime);
always @(reached_50)
    if (reached_50) $display("[%0t] *** reached_50 ***", $realtime);
always @(pulse_seen)
    if (pulse_seen) $display("[%0t] *** pulse_seen ***", $realtime);

initial begin
    $dumpfile("tb_ref.vcd");
    $dumpvars(0, tb_ref);
end

initial begin
    #(60_000);  // 60us (timescale 1ns/1ps)
    $display("");
    $display("=== reference TB finished at 60us ===");
    $display("  count      = %0d", dut.count);
    $display("  reached_10 = %0b", reached_10);
    $display("  reached_25 = %0b", reached_25);
    $display("  reached_50 = %0b", reached_50);
    $display("  pulse_seen = %0b", pulse_seen);
    $finish;
end

endmodule
