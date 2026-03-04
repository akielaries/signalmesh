//Copyright (C)2014-2025 Gowin Semiconductor Corporation.
//All rights reserved.
//File Title: Template file for instantiation
//Tool Version: V1.9.12.01
//IP Version: 1.0
//Part Number: GW5AT-LV60PG484AC2/I1
//Device: GW5AT-60
//Device Version: B
//Created Time: Sun Feb 15 19:19:56 2026

//Change the instance name and port connections to the signal names
//--------Copy here to design--------

	AHB_to_APB_32_Bridge_Top your_instance_name(
		.hclk(hclk), //input hclk
		.hresetn(hresetn), //input hresetn
		.hsel(hsel), //input hsel
		.hready_in(hready_in), //input hready_in
		.htrans(htrans), //input [1:0] htrans
		.haddr(haddr), //input [31:0] haddr
		.hsize(hsize), //input [2:0] hsize
		.hprot(hprot), //input [3:0] hprot
		.hwrite(hwrite), //input hwrite
		.hwdata(hwdata), //input [31:0] hwdata
		.apb2ahb_clken(apb2ahb_clken), //input apb2ahb_clken
		.hrdata(hrdata), //output [31:0] hrdata
		.hready(hready), //output hready
		.hresp(hresp), //output [1:0] hresp
		.pclk(pclk), //input pclk
		.presetn(presetn), //input presetn
		.pprot(pprot), //output [2:0] pprot
		.pstrb(pstrb), //output [3:0] pstrb
		.paddr(paddr), //output [31:0] paddr
		.penable(penable), //output penable
		.pwrite(pwrite), //output pwrite
		.pwdata(pwdata) //output [31:0] pwdata
	);

//--------Copy end-------------------
