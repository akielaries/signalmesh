//Copyright (C)2014-2025 Gowin Semiconductor Corporation.
//All rights reserved.
//File Title: Template file for instantiation
//Tool Version: V1.9.12.01
//IP Version: 2.1
//Part Number: GW1NR-LV9QN88PC6/I5
//Device: GW1NR-9
//Device Version: C
//Created Time: Tue Mar  3 19:46:00 2026

//Change the instance name and port connections to the signal names
//--------Copy here to design--------

	Gowin_EMPU_M1_Top your_instance_name(
		.LOCKUP(LOCKUP), //output LOCKUP
		.HALTED(HALTED), //output HALTED
		.GPIO(GPIO), //inout [15:0] GPIO
		.JTAG_7(JTAG_7), //inout JTAG_7
		.JTAG_9(JTAG_9), //inout JTAG_9
		.UART1RXD(UART1RXD), //input UART1RXD
		.UART1TXD(UART1TXD), //output UART1TXD
		.APB1PADDR(APB1PADDR), //output [31:0] APB1PADDR
		.APB1PENABLE(APB1PENABLE), //output APB1PENABLE
		.APB1PWRITE(APB1PWRITE), //output APB1PWRITE
		.APB1PSTRB(APB1PSTRB), //output [3:0] APB1PSTRB
		.APB1PPROT(APB1PPROT), //output [2:0] APB1PPROT
		.APB1PWDATA(APB1PWDATA), //output [31:0] APB1PWDATA
		.APB1PSEL(APB1PSEL), //output APB1PSEL
		.APB1PRDATA(APB1PRDATA), //input [31:0] APB1PRDATA
		.APB1PREADY(APB1PREADY), //input APB1PREADY
		.APB1PSLVERR(APB1PSLVERR), //input APB1PSLVERR
		.APB1PCLK(APB1PCLK), //output APB1PCLK
		.APB1PRESET(APB1PRESET), //output APB1PRESET
		.HCLK(HCLK), //input HCLK
		.hwRstn(hwRstn) //input hwRstn
	);

//--------Copy end-------------------
