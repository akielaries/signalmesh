/*
 * Sine Wave Generator Example for Gowin FPGAs
 *
 * This module demonstrates how to generate a sine wave using Pulse Width
 * Modulation (PWM). It produces a 30 kHz sine wave from a 27 MHz master clock.
 *
 * IMPORTANT: The output of this module is a PWM signal. To see an analog
 * sine wave, you MUST connect an external low-pass RC filter to the
 * output pin (bank2_3v3_waveform). A 1k Ohm resistor followed by a
 * 3.3nF capacitor to ground is a good starting point.
 */

module sine_wave_gen (
  // --- Inputs ---
  input bank1_3v3_xtal_in, // Input clock from the crystal oscillator (27 MHz)
  input bank3_1v8_sys_rst, // Active-low reset

  // --- Outputs ---
  output bank1_3v3_xtal_route,
  output bank2_3v3_waveform // The PWM signal output
);

  // --- Port Aliases (standard Verilog via wire/assign) ---
  wire clk;
  wire rst_n;

  assign clk = bank1_3v3_xtal_in;
  assign bank1_3v3_xtal_route = bank1_3v3_xtal_in;
  assign rst_n = bank3_1v8_sys_rst;
  // --------------------------------------------------------

  // --- Phase Accumulator for LUT Index ---
  // This steps through the sine lookup table at a rate that produces
  // the desired output frequency.
  // F_out = (F_clk * Step_inc) / (2^32)
  // Step_inc = (F_out * 2^32) / F_clk
  //          = (30,000 * 2^32) / 27,000,000 = 4772186
  localparam PHASE_INCREMENT = 32'd4772186;
  
  reg [31:0] phase_accumulator;
  always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      phase_accumulator <= 32'd0;
    end else begin
      phase_accumulator <= phase_accumulator + PHASE_INCREMENT;
    end
  end

  // Use the top 8 bits of the accumulator as the address for our 256-entry table.
  wire [7:0] lut_address = phase_accumulator[31:24];


  // --- Sine Wave Lookup Table (LUT) ---
  // This is a 256-entry, 8-bit Read-Only Memory (ROM) that stores one
  // cycle of a sine wave. The values range from 0 to 255.
  reg [7:0] lut_value;
  always @(*) begin
    case (lut_address)
      8'd0:   lut_value = 8'd128;
      8'd1:   lut_value = 8'd131;
      8'd2:   lut_value = 8'd134;
      8'd3:   lut_value = 8'd137;
      8'd4:   lut_value = 8'd140;
      8'd5:   lut_value = 8'd143;
      8'd6:   lut_value = 8'd146;
      8'd7:   lut_value = 8'd149;
      8'd8:   lut_value = 8'd152;
      8'd9:   lut_value = 8'd155;
      8'd10:  lut_value = 8'd158;
      8'd11:  lut_value = 8'd161;
      8'd12:  lut_value = 8'd164;
      8'd13:  lut_value = 8'd167;
      8'd14:  lut_value = 8'd170;
      8'd15:  lut_value = 8'd173;
      8'd16:  lut_value = 8'd176;
      8'd17:  lut_value = 8'd179;
      8'd18:  lut_value = 8'd182;
      8'd19:  lut_value = 8'd185;
      8'd20:  lut_value = 8'd188;
      8'd21:  lut_value = 8'd190;
      8'd22:  lut_value = 8'd193;
      8'd23:  lut_value = 8'd196;
      8'd24:  lut_value = 8'd198;
      8'd25:  lut_value = 8'd201;
      8'd26:  lut_value = 8'd203;
      8'd27:  lut_value = 8'd206;
      8'd28:  lut_value = 8'd208;
      8'd29:  lut_value = 8'd211;
      8'd30:  lut_value = 8'd213;
      8'd31:  lut_value = 8'd215;
      8'd32:  lut_value = 8'd217;
      8'd33:  lut_value = 8'd219;
      8'd34:  lut_value = 8'd221;
      8'd35:  lut_value = 8'd223;
      8'd36:  lut_value = 8'd225;
      8'd37:  lut_value = 8'd227;
      8'd38:  lut_value = 8'd229;
      8'd39:  lut_value = 8'd231;
      8'd40:  lut_value = 8'd232;
      8'd41:  lut_value = 8'd234;
      8'd42:  lut_value = 8'd236;
      8'd43:  lut_value = 8'd237;
      8'd44:  lut_value = 8'd239;
      8'd45:  lut_value = 8'd240;
      8'd46:  lut_value = 8'd241;
      8'd47:  lut_value = 8'd243;
      8'd48:  lut_value = 8'd244;
      8'd49:  lut_value = 8'd245;
      8'd50:  lut_value = 8'd246;
      8'd51:  lut_value = 8'd247;
      8'd52:  lut_value = 8'd248;
      8'd53:  lut_value = 8'd249;
      8'd54:  lut_value = 8'd250;
      8'd55:  lut_value = 8'd251;
      8'd56:  lut_value = 8'd252;
      8'd57:  lut_value = 8'd252;
      8'd58:  lut_value = 8'd253;
      8'd59:  lut_value = 8'd253;
      8'd60:  lut_value = 8'd254;
      8'd61:  lut_value = 8'd254;
      8'd62:  lut_value = 8'd255;
      8'd63:  lut_value = 8'd255;
      8'd64:  lut_value = 8'd255;
      8'd65:  lut_value = 8'd255;
      8'd66:  lut_value = 8'd255;
      8'd67:  lut_value = 8'd254;
      8'd68:  lut_value = 8'd254;
      8'd69:  lut_value = 8'd253;
      8'd70:  lut_value = 8'd253;
      8'd71:  lut_value = 8'd252;
      8'd72:  lut_value = 8'd252;
      8'd73:  lut_value = 8'd251;
      8'd74:  lut_value = 8'd250;
      8'd75:  lut_value = 8'd249;
      8'd76:  lut_value = 8'd248;
      8'd77:  lut_value = 8'd247;
      8'd78:  lut_value = 8'd246;
      8'd79:  lut_value = 8'd245;
      8'd80:  lut_value = 8'd244;
      8'd81:  lut_value = 8'd243;
      8'd82:  lut_value = 8'd241;
      8'd83:  lut_value = 8'd240;
      8'd84:  lut_value = 8'd239;
      8'd85:  lut_value = 8'd237;
      8'd86:  lut_value = 8'd236;
      8'd87:  lut_value = 8'd234;
      8'd88:  lut_value = 8'd232;
      8'd89:  lut_value = 8'd231;
      8'd90:  lut_value = 8'd229;
      8'd91:  lut_value = 8'd227;
      8'd92:  lut_value = 8'd225;
      8'd93:  lut_value = 8'd223;
      8'd94:  lut_value = 8'd221;
      8'd95:  lut_value = 8'd219;
      8'd96:  lut_value = 8'd217;
      8'd97:  lut_value = 8'd215;
      8'd98:  lut_value = 8'd213;
      8'd99:  lut_value = 8'd211;
      8'd100: lut_value = 8'd208;
      8'd101: lut_value = 8'd206;
      8'd102: lut_value = 8'd203;
      8'd103: lut_value = 8'd201;
      8'd104: lut_value = 8'd198;
      8'd105: lut_value = 8'd196;
      8'd106: lut_value = 8'd193;
      8'd107: lut_value = 8'd190;
      8'd108: lut_value = 8'd188;
      8'd109: lut_value = 8'd185;
      8'd110: lut_value = 8'd182;
      8'd111: lut_value = 8'd179;
      8'd112: lut_value = 8'd176;
      8'd113: lut_value = 8'd173;
      8'd114: lut_value = 8'd170;
      8'd115: lut_value = 8'd167;
      8'd116: lut_value = 8'd164;
      8'd117: lut_value = 8'd161;
      8'd118: lut_value = 8'd158;
      8'd119: lut_value = 8'd155;
      8'd120: lut_value = 8'd152;
      8'd121: lut_value = 8'd149;
      8'd122: lut_value = 8'd146;
      8'd123: lut_value = 8'd143;
      8'd124: lut_value = 8'd140;
      8'd125: lut_value = 8'd137;
      8'd126: lut_value = 8'd134;
      8'd127: lut_value = 8'd131;
      8'd128: lut_value = 8'd128;
      8'd129: lut_value = 8'd125;
      8'd130: lut_value = 8'd122;
      8'd131: lut_value = 8'd119;
      8'd132: lut_value = 8'd116;
      8'd133: lut_value = 8'd113;
      8'd134: lut_value = 8'd110;
      8'd135: lut_value = 8'd107;
      8'd136: lut_value = 8'd104;
      8'd137: lut_value = 8'd101;
      8'd138: lut_value = 8'd98;
      8'd139: lut_value = 8'd95;
      8'd140: lut_value = 8'd92;
      8'd141: lut_value = 8'd89;
      8'd142: lut_value = 8'd86;
      8'd143: lut_value = 8'd83;
      8'd144: lut_value = 8'd80;
      8'd145: lut_value = 8'd77;
      8'd146: lut_value = 8'd74;
      8'd147: lut_value = 8'd71;
      8'd148: lut_value = 8'd68;
      8'd149: lut_value = 8'd66;
      8'd150: lut_value = 8'd63;
      8'd151: lut_value = 8'd60;
      8'd152: lut_value = 8'd58;
      8'd153: lut_value = 8'd55;
      8'd154: lut_value = 8'd53;
      8'd155: lut_value = 8'd50;
      8'd156: lut_value = 8'd48;
      8'd157: lut_value = 8'd45;
      8'd158: lut_value = 8'd43;
      8'd159: lut_value = 8'd41;
      8'd160: lut_value = 8'd39;
      8'd161: lut_value = 8'd37;
      8'd162: lut_value = 8'd35;
      8'd163: lut_value = 8'd33;
      8'd164: lut_value = 8'd31;
      8'd165: lut_value = 8'd29;
      8'd166: lut_value = 8'd27;
      8'd167: lut_value = 8'd25;
      8'd168: lut_value = 8'd24;
      8'd169: lut_value = 8'd22;
      8'd170: lut_value = 8'd20;
      8'd171: lut_value = 8'd19;
      8'd172: lut_value = 8'd17;
      8'd173: lut_value = 8'd16;
      8'd174: lut_value = 8'd15;
      8'd175: lut_value = 8'd13;
      8'd176: lut_value = 8'd12;
      8'd177: lut_value = 8'd11;
      8'd178: lut_value = 8'd10;
      8'd179: lut_value = 8'd9;
      8'd180: lut_value = 8'd8;
      8'd181: lut_value = 8'd7;
      8'd182: lut_value = 8'd6;
      8'd183: lut_value = 8'd5;
      8'd184: lut_value = 8'd4;
      8'd185: lut_value = 8'd4;
      8'd186: lut_value = 8'd3;
      8'd187: lut_value = 8'd3;
      8'd188: lut_value = 8'd2;
      8'd189: lut_value = 8'd2;
      8'd190: lut_value = 8'd1;
      8'd191: lut_value = 8'd1;
      8'd192: lut_value = 8'd1;
      8'd193: lut_value = 8'd1;
      8'd194: lut_value = 8'd1;
      8'd195: lut_value = 8'd2;
      8'd196: lut_value = 8'd2;
      8'd197: lut_value = 8'd3;
      8'd198: lut_value = 8'd3;
      8'd199: lut_value = 8'd4;
      8'd200: lut_value = 8'd4;
      8'd201: lut_value = 8'd5;
      8'd202: lut_value = 8'd6;
      8'd203: lut_value = 8'd7;
      8'd204: lut_value = 8'd8;
      8'd205: lut_value = 8'd9;
      8'd206: lut_value = 8'd10;
      8'd207: lut_value = 8'd11;
      8'd208: lut_value = 8'd12;
      8'd209: lut_value = 8'd13;
      8'd210: lut_value = 8'd15;
      8'd211: lut_value = 8'd16;
      8'd212: lut_value = 8'd17;
      8'd213: lut_value = 8'd19;
      8'd214: lut_value = 8'd20;
      8'd215: lut_value = 8'd22;
      8'd216: lut_value = 8'd24;
      8'd217: lut_value = 8'd25;
      8'd218: lut_value = 8'd27;
      8'd219: lut_value = 8'd29;
      8'd220: lut_value = 8'd31;
      8'd221: lut_value = 8'd33;
      8'd222: lut_value = 8'd35;
      8'd223: lut_value = 8'd37;
      8'd224: lut_value = 8'd39;
      8'd225: lut_value = 8'd41;
      8'd226: lut_value = 8'd43;
      8'd227: lut_value = 8'd45;
      8'd228: lut_value = 8'd48;
      8'd229: lut_value = 8'd50;
      8'd230: lut_value = 8'd53;
      8'd231: lut_value = 8'd55;
      8'd232: lut_value = 8'd58;
      8'd233: lut_value = 8'd60;
      8'd234: lut_value = 8'd63;
      8'd235: lut_value = 8'd66;
      8'd236: lut_value = 8'd68;
      8'd237: lut_value = 8'd71;
      8'd238: lut_value = 8'd74;
      8'd239: lut_value = 8'd77;
      8'd240: lut_value = 8'd80;
      8'd241: lut_value = 8'd83;
      8'd242: lut_value = 8'd86;
      8'd243: lut_value = 8'd89;
      8'd244: lut_value = 8'd92;
      8'd245: lut_value = 8'd95;
      8'd246: lut_value = 8'd98;
      8'd247: lut_value = 8'd101;
      8'd248: lut_value = 8'd104;
      8'd249: lut_value = 8'd107;
      8'd250: lut_value = 8'd110;
      8'd251: lut_value = 8'd113;
      8'd252: lut_value = 8'd116;
      8'd253: lut_value = 8'd119;
      8'd254: lut_value = 8'd122;
      8'd255: lut_value = 8'd125;
      default: lut_value = 8'd128;
    endcase
  end

  // --- PWM Generator ---
  // This creates a high-frequency square wave whose duty cycle is
  // controlled by the value from the sine LUT.
  reg [7:0] pwm_counter;
  always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      pwm_counter <= 8'd0;
    end else begin
      // The counter continuously cycles from 0-255
      pwm_counter <= pwm_counter + 1'b1;
    end
  end

  // The output is high when the counter is less than the sine value.
  // This creates the variable duty cycle.
  assign bank2_3v3_waveform = (pwm_counter < lut_value);

endmodule
