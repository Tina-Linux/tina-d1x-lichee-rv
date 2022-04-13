/* drivers/video/sunxi/disp2/disp/lcd/tft08006.c
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * tft08006 panel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 &lcd0 {
	lcd_used            = <1>;

	lcd_driver_name     = "tft08006";
	lcd_backlight       = <50>;
	lcd_if              = <4>;

	lcd_x               = <800>;
	lcd_y               = <1280>;
	lcd_width           = <52>;
	lcd_height          = <52>;
	lcd_dclk_freq       = <68>;

	lcd_pwm_used        = <1>;
	lcd_pwm_ch          = <2>;
	lcd_pwm_freq        = <50000>;
	lcd_pwm_pol         = <1>;
	lcd_pwm_max_limit   = <255>;

	lcd_hbp             = <40>;
	lcd_ht              = <860>;
	lcd_hspw            = <20>;
	lcd_vbp             = <24>;
	lcd_vt              = <1330>;
	lcd_vspw            = <4>;

	lcd_dsi_if          = <0>;
	lcd_dsi_lane        = <4>;
	lcd_lvds_if         = <0>;
	lcd_lvds_colordepth = <0>;
	lcd_lvds_mode       = <0>;
	lcd_frm             = <0>;
	lcd_hv_clk_phase    = <0>;
	lcd_hv_sync_polarity= <0>;
	lcd_gamma_en        = <0>;
	lcd_bright_curve_en = <0>;
	lcd_cmap_en         = <0>;
	lcd_fsync_en        = <0>;
	lcd_fsync_act_time  = <1000>;
	lcd_fsync_dis_time  = <1000>;
	lcd_fsync_pol       = <0>;

	deu_mode            = <0>;
	lcdgamma4iep        = <22>;
	smart_color         = <90>;

	lcd_gpio_0 =  <&pio PG 13 GPIO_ACTIVE_HIGH>;
	pinctrl-0 = <&dsi4lane_pins_a>;
	pinctrl-1 = <&dsi4lane_pins_b>;
};
*/
#include "tft08006.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		{ 0, 0 },     { 15, 15 },   { 30, 30 },   { 45, 45 },
		{ 60, 60 },   { 75, 75 },   { 90, 90 },   { 105, 105 },
		{ 120, 120 }, { 135, 135 }, { 150, 150 }, { 165, 165 },
		{ 180, 180 }, { 195, 195 }, { 210, 210 }, { 225, 225 },
		{ 240, 240 }, { 255, 255 },
	};

	u32 lcd_cmap_tbl[2][3][4] = {
		{
			{ LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3 },
			{ LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3 },
			{ LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3 },
		},
		{
			{ LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0 },
			{ LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0 },
			{ LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0 },
		},
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
				((lcd_gamma_tbl[i + 1][1] -
				  lcd_gamma_tbl[i][1]) *
				 j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
				(value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) +
				   (lcd_gamma_tbl[items - 1][1] << 8) +
				   lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
}

static s32 lcd_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, lcd_power_on, 10);
	LCD_OPEN_FUNC(sel, lcd_panel_init, 10);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);
	LCD_OPEN_FUNC(sel, lcd_bl_open, 0);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, lcd_bl_close, 0);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 200);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 500);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(10);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(5);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_delay_ms(20);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(5);
}

static void lcd_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
}

static void lcd_bl_close(u32 sel)
{
	sunxi_lcd_pwm_disable(sel);
}

#define REGFLAG_DELAY 0XFC
#define REGFLAG_END_OF_TABLE 0xFD /* END OF REGISTERS MARKER */

struct LCM_setting_table {
	u8 cmd;
	u32 count;
	u8 para_list[32];
};

static struct LCM_setting_table lcm_tft08006_setting[] = {
	{ 0xB0, 1, { 0x5A } },

	{ 0xB1, 1, { 0x00 } },
	{ 0x89, 1, { 0x01 } },
	{ 0x91, 1, { 0x17 } },
	{ 0xB1, 1, { 0x03 } },
	{ 0x2C, 1, { 0x28 } },

	{ 0x00, 1, { 0xB7 } },
	{ 0x01, 1, { 0x1B } },
	{ 0x02, 1, { 0x00 } },
	{ 0x03, 1, { 0x00 } },
	{ 0x04, 1, { 0x00 } },
	{ 0x05, 1, { 0x00 } },
	{ 0x06, 1, { 0x00 } },
	{ 0x07, 1, { 0x00 } },
	{ 0x08, 1, { 0x00 } },
	{ 0x09, 1, { 0x00 } },
	{ 0x0A, 1, { 0x01 } },
	{ 0x0B, 1, { 0x01 } },
	{ 0x0C, 1, { 0x00 } },
	{ 0x0D, 1, { 0x00 } },
	{ 0x0E, 1, { 0x24 } },
	{ 0x0F, 1, { 0x1C } },
	{ 0x10, 1, { 0xC9 } },
	{ 0x11, 1, { 0x60 } },
	{ 0x12, 1, { 0x70 } },
	{ 0x13, 1, { 0x01 } },
	{ 0x14, 1, { 0xE7 } },
	{ 0x15, 1, { 0xFF } },
	{ 0x16, 1, { 0x3D } },
	{ 0x17, 1, { 0x0E } },
	{ 0x18, 1, { 0x01 } },
	{ 0x19, 1, { 0x00 } },
	{ 0x1A, 1, { 0x00 } },
	{ 0x1B, 1, { 0xFC } },
	{ 0x1C, 1, { 0x0B } },
	{ 0x1D, 1, { 0xA0 } },
	{ 0x1E, 1, { 0x03 } },
	{ 0x1F, 1, { 0x04 } },
	{ 0x20, 1, { 0x0C } },
	{ 0x21, 1, { 0x00 } },
	{ 0x22, 1, { 0x04 } },
	{ 0x23, 1, { 0x81 } },
	{ 0x24, 1, { 0x1F } },
	{ 0x25, 1, { 0x10 } },
	{ 0x26, 1, { 0x9B } },
	{ 0x2D, 1, { 0x01 } },
	{ 0x2E, 1, { 0x84 } },
	{ 0x2F, 1, { 0x00 } },
	{ 0x30, 1, { 0x02 } },
	{ 0x31, 1, { 0x08 } },
	{ 0x32, 1, { 0x01 } },
	{ 0x33, 1, { 0x1C } },
	{ 0x34, 1, { 0x40 } },
	{ 0x35, 1, { 0xFF } },
	{ 0x36, 1, { 0xFF } },
	{ 0x37, 1, { 0xFF } },
	{ 0x38, 1, { 0xFF } },
	{ 0x39, 1, { 0xFF } },
	{ 0x3A, 1, { 0x05 } },
	{ 0x3B, 1, { 0x00 } },
	{ 0x3C, 1, { 0x00 } },
	{ 0x3D, 1, { 0x00 } },
	{ 0x3E, 1, { 0xCF } },
	{ 0x3F, 1, { 0x84 } },
	{ 0x40, 1, { 0x2F } },
	{ 0x41, 1, { 0xFC } },
	{ 0x42, 1, { 0x01 } },
	{ 0x43, 1, { 0x40 } },
	{ 0x44, 1, { 0x05 } },
	{ 0x45, 1, { 0xE8 } },
	{ 0x46, 1, { 0x16 } },
	{ 0x47, 1, { 0x00 } },
	{ 0x48, 1, { 0x00 } },
	{ 0x49, 1, { 0x88 } },
	{ 0x4A, 1, { 0x08 } },
	{ 0x4B, 1, { 0x05 } },
	{ 0x4C, 1, { 0x03 } },
	{ 0x4D, 1, { 0xD0 } },
	{ 0x4E, 1, { 0x13 } },
	{ 0x4F, 1, { 0xFF } },
	{ 0x50, 1, { 0x0A } },
	{ 0x51, 1, { 0x53 } },
	{ 0x52, 1, { 0x26 } },
	{ 0x53, 1, { 0x22 } },
	{ 0x54, 1, { 0x09 } },
	{ 0x55, 1, { 0x22 } },
	{ 0x56, 1, { 0x00 } },
	{ 0x57, 1, { 0x1C } },
	{ 0x58, 1, { 0x03 } },
	{ 0x59, 1, { 0x3F } },
	{ 0x5A, 1, { 0x28 } },
	{ 0x5B, 1, { 0x01 } },
	{ 0x5C, 1, { 0xCC } },
	{ 0x5D, 1, { 0x21 } },
	{ 0x5E, 1, { 0x84 } },
	{ 0x5F, 1, { 0x10 } },
	{ 0x60, 1, { 0x42 } },
	{ 0x61, 1, { 0x08 } },
	{ 0x62, 1, { 0x21 } },
	{ 0x63, 1, { 0x84 } },
	{ 0x64, 1, { 0x80 } },
	{ 0x65, 1, { 0x0C } },
	{ 0x66, 1, { 0x74 } },
	{ 0x67, 1, { 0x4C } },
	{ 0x68, 1, { 0x09 } },
	{ 0x69, 1, { 0x12 } },
	{ 0x6A, 1, { 0x42 } },
	{ 0x6B, 1, { 0x08 } },
	{ 0x6C, 1, { 0x21 } },
	{ 0x6D, 1, { 0x84 } },
	{ 0x6E, 1, { 0x10 } },
	{ 0x6F, 1, { 0x42 } },
	{ 0x70, 1, { 0x08 } },
	{ 0x71, 1, { 0xE9 } },
	{ 0x72, 1, { 0xC4 } },
	{ 0x73, 1, { 0xD7 } },
	{ 0x74, 1, { 0xD6 } },
	{ 0x75, 1, { 0x28 } },
	{ 0x76, 1, { 0x00 } },
	{ 0x77, 1, { 0x00 } },
	{ 0x78, 1, { 0x0F } },
	{ 0x79, 1, { 0xE0 } },
	{ 0x7A, 1, { 0x01 } },
	{ 0x7B, 1, { 0xFF } },
	{ 0x7C, 1, { 0xFF } },
	{ 0x7D, 1, { 0x0F } },
	{ 0x7E, 1, { 0x41 } },
	{ 0x7F, 1, { 0xFE } },

	{ 0xB1, 1, { 0x02 } },
	{ 0x00, 1, { 0xFF } },
	{ 0x01, 1, { 0x05 } },
	{ 0x02, 1, { 0xF0 } },
	{ 0x03, 1, { 0x00 } },
	{ 0x04, 1, { 0x94 } },
	{ 0x05, 1, { 0x48 } },
	{ 0x06, 1, { 0xC8 } },
	{ 0x07, 1, { 0x0A } },
	{ 0x08, 1, { 0x00 } },
	{ 0x09, 1, { 0x00 } },
	{ 0x0A, 1, { 0x00 } },
	{ 0x0B, 1, { 0x10 } },
	{ 0x0C, 1, { 0xE6 } },
	{ 0x0D, 1, { 0x0D } },
	{ 0x0F, 1, { 0x00 } },
	{ 0x10, 1, { 0xF9 } },
	{ 0x11, 1, { 0x4D } },
	{ 0x12, 1, { 0x9E } },
	{ 0x13, 1, { 0x8F } },
	{ 0x14, 1, { 0xDF } },
	{ 0x15, 1, { 0x15 } },
	{ 0x16, 1, { 0xE4 } },
	{ 0x17, 1, { 0x6A } },
	{ 0x18, 1, { 0xAB } },
	{ 0x19, 1, { 0xD7 } },
	{ 0x1A, 1, { 0x78 } },
	{ 0x1B, 1, { 0xFE } },
	{ 0x1C, 1, { 0xFF } },
	{ 0x1D, 1, { 0xFF } },
	{ 0x1E, 1, { 0xFF } },
	{ 0x1F, 1, { 0xFF } },
	{ 0x20, 1, { 0xFF } },
	{ 0x21, 1, { 0xFF } },
	{ 0x22, 1, { 0xFF } },
	{ 0x23, 1, { 0xFF } },
	{ 0x24, 1, { 0xFF } },
	{ 0x25, 1, { 0xFF } },
	{ 0x26, 1, { 0xFF } },
	{ 0x27, 1, { 0x1F } },
	{ 0x28, 1, { 0xFF } },
	{ 0x29, 1, { 0xFF } },
	{ 0x2A, 1, { 0xFF } },
	{ 0x2B, 1, { 0xFF } },
	{ 0x2C, 1, { 0xFF } },
	{ 0x2D, 1, { 0x07 } },
	{ 0x33, 1, { 0x3F } },
	{ 0x35, 1, { 0x7F } },
	{ 0x36, 1, { 0x3F } },
	{ 0x38, 1, { 0xFF } },
	{ 0x3A, 1, { 0x80 } },
	{ 0x3B, 1, { 0x01 } },
	{ 0x3C, 1, { 0x00 } },
	{ 0x3D, 1, { 0x2F } },
	{ 0x3E, 1, { 0x00 } },
	{ 0x3F, 1, { 0xE0 } },
	{ 0x40, 1, { 0x05 } },
	{ 0x41, 1, { 0x00 } },
	{ 0x42, 1, { 0xBC } },
	{ 0x43, 1, { 0x00 } },
	{ 0x44, 1, { 0x80 } },
	{ 0x45, 1, { 0x07 } },
	{ 0x46, 1, { 0x00 } },
	{ 0x47, 1, { 0x00 } },
	{ 0x48, 1, { 0x9B } },
	{ 0x49, 1, { 0xD2 } },
	{ 0x4A, 1, { 0xC1 } },
	{ 0x4B, 1, { 0x83 } },
	{ 0x4C, 1, { 0x17 } },
	{ 0x4D, 1, { 0xC0 } },
	{ 0x4E, 1, { 0x0F } },
	{ 0x4F, 1, { 0xF1 } },
	{ 0x50, 1, { 0x78 } },
	{ 0x51, 1, { 0x7A } },
	{ 0x52, 1, { 0x34 } },
	{ 0x53, 1, { 0x99 } },
	{ 0x54, 1, { 0xA2 } },
	{ 0x55, 1, { 0x02 } },
	{ 0x56, 1, { 0x14 } },
	{ 0x57, 1, { 0xB8 } },
	{ 0x58, 1, { 0xDC } },
	{ 0x59, 1, { 0xD4 } },
	{ 0x5A, 1, { 0xEF } },
	{ 0x5B, 1, { 0xF7 } },
	{ 0x5C, 1, { 0xFB } },
	{ 0x5D, 1, { 0xFD } },
	{ 0x5E, 1, { 0x7E } },
	{ 0x5F, 1, { 0xBF } },
	{ 0x60, 1, { 0xEF } },
	{ 0x61, 1, { 0xE6 } },
	{ 0x62, 1, { 0x76 } },
	{ 0x63, 1, { 0x73 } },
	{ 0x64, 1, { 0xBB } },
	{ 0x65, 1, { 0xDD } },
	{ 0x66, 1, { 0x6E } },
	{ 0x67, 1, { 0x37 } },
	{ 0x68, 1, { 0x8C } },
	{ 0x69, 1, { 0x08 } },
	{ 0x6A, 1, { 0x31 } },
	{ 0x6B, 1, { 0xB8 } },
	{ 0x6C, 1, { 0xB8 } },
	{ 0x6D, 1, { 0xB8 } },
	{ 0x6E, 1, { 0xB8 } },
	{ 0x6F, 1, { 0xB8 } },
	{ 0x70, 1, { 0x5C } },
	{ 0x71, 1, { 0x2E } },
	{ 0x72, 1, { 0x17 } },
	{ 0x73, 1, { 0x00 } },
	{ 0x74, 1, { 0x00 } },
	{ 0x75, 1, { 0x00 } },
	{ 0x76, 1, { 0x00 } },
	{ 0x77, 1, { 0x00 } },
	{ 0x78, 1, { 0x00 } },
	{ 0x79, 1, { 0x00 } },
	{ 0x7A, 1, { 0xDC } },
	{ 0x7B, 1, { 0xDC } },
	{ 0x7C, 1, { 0xDC } },
	{ 0x7D, 1, { 0xDC } },
	{ 0x7E, 1, { 0xDC } },
	{ 0x7F, 1, { 0x6E } },
	{ 0x0B, 1, { 0x00 } },
	{ 0xB1, 1, { 0x03 } },
	{ 0x2C, 1, { 0x2C } },
	{ 0xB1, 1, { 0x00 } },
	{ 0x89, 1, { 0x03 } },

	{ REGFLAG_DELAY, REGFLAG_DELAY, { 20 } },

	{ REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {} }
};

static void lcd_panel_init(u32 sel)
{
	__u32 i;
	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(20);
	for (i = 0;; i++) {
		if (lcm_tft08006_setting[i].count == REGFLAG_END_OF_TABLE)
			break;
		else if (lcm_tft08006_setting[i].count == REGFLAG_DELAY) {
			sunxi_lcd_delay_ms(
				lcm_tft08006_setting[i].para_list[0]);
		} else {
			dsi_gen_wr(sel, lcm_tft08006_setting[i].cmd,
				   lcm_tft08006_setting[i].para_list,
				   lcm_tft08006_setting[i].count);
		}
		/* break; */
	}
}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x10);
	sunxi_lcd_delay_ms(80);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x28);
	sunxi_lcd_delay_ms(50);
}

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

__lcd_panel_t tft08006_panel = {
	/* panel driver name, must mach the name of
	 * lcd_drv_name in sys_config.fex
	 */
	.name = "tft08006",
	.func =
		{
			.cfg_panel_info	= lcd_cfg_panel_info,
			.cfg_open_flow	 = lcd_open_flow,
			.cfg_close_flow	= lcd_close_flow,
			.lcd_user_defined_func = lcd_user_defined_func,
		},
};
