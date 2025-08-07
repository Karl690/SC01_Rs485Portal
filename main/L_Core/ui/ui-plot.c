#include "../devices/display.h"
#include <dirent.h> 
#include <stdio.h>
#include "ui.h"
#include "ui-plot.h"
#include "main.h"
#include "RevisionHistory.h"
#include "K_Core/serial/serial.h"
#include "K_Core/execution/cmdprocessor.h"
#include "L_Core/bluetooth/ble.h"

lv_obj_t* ui_plot_screen;
lv_obj_t* ui_plot_canvas;
uint16_t ui_plot_width = SCREEN_WIDTH;
uint16_t ui_plot_height = SCREEN_HEIGHT;

lv_obj_t* ui_plot_panel;
UI_PLOT_OBJ ui_plot_obj;
uint16_t ui_plot_scan_points[UI_PLOT_CHANNEL_NUM][UI_PLOT_MAX_POINTS];
uint32_t ui_plot_channel_color[UI_PLOT_CHANNEL_NUM] = { 
	UI_PLOT_COLOR_CH0,
	UI_PLOT_COLOR_CH1,
	UI_PLOT_COLOR_CH2,
	UI_PLOT_COLOR_CH3,
	UI_PLOT_COLOR_CH4,
	UI_PLOT_COLOR_CH5,
};

UI_PLOT_INFO ui_plot_info = {
	.min_x = 9000,
	.max_x = 10000,
	.min_y = 0,
	.max_y = 1000,
	.step_xn = 200,
	.step_yn = 200,
	.channel_visible = {1, 1, 1,1,1,1}
};
	
void ui_plot_clear()
{
	memset(ui_plot_scan_points, 0, sizeof(uint16_t)*UI_PLOT_CHANNEL_NUM * UI_PLOT_MAX_POINTS);
	memset(ui_plot_info.channel_peaks, 0, sizeof(lv_point_t) * UI_PLOT_CHANNEL_NUM);
	ui_plot_axis(ui_plot_panel, false);
}

void ui_plot_zoom(bool inout)
{
	if (inout) // zoom in
	{
		ui_plot_info.max_y += 100;
		ui_plot_info.step_yn = ((ui_plot_info.max_y - ui_plot_info.min_y) / 50) * 10;
		
	}
	else // zoom out
	{
		if (ui_plot_info.max_y - 100 > 500)
		{
			ui_plot_info.max_y -= 100;
			ui_plot_info.step_yn = ((ui_plot_info.max_y - ui_plot_info.min_y) / 50) * 10;
		}
	}
	ui_plot_refresh_by_buffer();
}

void ui_plot_call_event_button(uint8_t code, bool direct)
{
	ui_send_button_event(SCREEN_TUNE, code, direct);
	switch (code)
	{
	case UI_PLOT_BTN_SCAN:
		communication_tx_commandline(MasterCommPort, "M801 I666 T0 C0\n");
		break;
	case UI_PLOT_BTN_STOP:
		communication_tx_commandline(MasterCommPort, "M801 I661 T255\n");
		break;
	case UI_PLOT_BTN_ANALYZE:
		ui_plot_analyze();
		break;
	case UI_PLOT_BTN_PEAKS_APPLY:
		ui_plot_peaks();
		ui_plot_APPLY_SendFreqTo407();
		break;
	case UI_PLOT_BTN_SAVE:
		switch (systemconfig.can_address)
		{
			//flash the current settings to the correct amplifier,
			//each amplifier has 6 channels , C6 says to flash all 6 channels,
		case 0:communication_tx_commandline(MasterCommPort, "M801 I665 T0 C6\n"); break;
		case 6:communication_tx_commandline(MasterCommPort, "M801 I665 T6 C6\n"); break;
		case 12:communication_tx_commandline(MasterCommPort, "M801 I665 T12 C6\n"); break;
		case 18:communication_tx_commandline(MasterCommPort, "M801 I665 T18 C6\n"); break;
		case 24:communication_tx_commandline(MasterCommPort, "M801 I665 T24 C6\n"); break;
		case 30:communication_tx_commandline(MasterCommPort, "M801 I665 T30 C6\n"); break;
		default:communication_tx_commandline(MasterCommPort, "M801 I665 T0 C6\n"); break;
		}
		break;
		
	case UI_PLOT_BTN_ZOOMIN:
		ui_plot_zoom(true);
		break;
	case UI_PLOT_BTN_ZOOMOUT:
		ui_plot_zoom(false);
		break;
	case UI_PLOT_BTN_CHANNLE_00:
	case UI_PLOT_BTN_CHANNLE_01:
	case UI_PLOT_BTN_CHANNLE_02:
	case UI_PLOT_BTN_CHANNLE_03:
	case UI_PLOT_BTN_CHANNLE_04:
	case UI_PLOT_BTN_CHANNLE_05:
		if (cmd_tuning) break;
		code -= UI_PLOT_BTN_CHANNLE_00;
		ui_plot_info.channel_visible[code] = !ui_plot_info.channel_visible[code];
		lv_obj_set_style_bg_color(ui_plot_obj.button_ch[code], lv_color_hex(ui_plot_info.channel_visible[code] ? ui_plot_channel_color[code] : 0x333333), LV_PART_MAIN);
		ui_plot_refresh_by_buffer();
		break;
	}
}
void ui_plot_event_button_cb(lv_event_t* e)
{
	uint8_t code = (uint8_t)(int)lv_event_get_user_data(e);
	ui_plot_call_event_button(code, true);
}


void ui_event_canvas_cb(lv_event_t* e)
{
	lv_point_t point;
	lv_indev_get_point(lv_indev_get_act(), &point);
	if (point.x < ui_plot_width - 50) return;
	if (point.y < 100)
	{
		ui_plot_call_event_button(UI_PLOT_BTN_ZOOMOUT, true);
	}
	else if (point.y > ui_plot_height - 50)
	{
		ui_plot_call_event_button(UI_PLOT_BTN_ZOOMIN, true);
	}
}

lv_point_t ui_plot_pos2value(lv_point_t pos)
{
	lv_point_t val;
	float rx = (ui_plot_info.max_x - ui_plot_info.min_x) / (float)ui_plot_width;
	val.x =  pos.x * rx + ui_plot_info.min_x;
	float ry = (ui_plot_info.max_y - ui_plot_info.min_y) / (float)ui_plot_height;
	val.y =  (ui_plot_height - pos.y) * ry + ui_plot_info.min_y;
	return val;
}

lv_point_t ui_plot_val2pos(lv_point_t val)
{
	lv_point_t pos;
	float rx = (ui_plot_info.max_x - ui_plot_info.min_x) / (float)ui_plot_width;
	pos.x =  (val.x - ui_plot_info.min_x) / rx;
	float ry = (ui_plot_info.max_y - ui_plot_info.min_y) / (float)ui_plot_height;
	pos.y =  ui_plot_height - (val.y - ui_plot_info.min_y) / ry;
	return pos;
}

void ui_plot_update_timer(lv_timer_t * timer)
{
	if (cmd_report_head == cmd_report_tail) return;
	lv_point_t value, pos;
	lv_draw_arc_dsc_t arc_dsc;
	lv_draw_arc_dsc_init(&arc_dsc);
	FILE* fp = NULL;
	if (systemconfig.sdcard.status)
	{
		fp = fopen(SDCARD_MOUNT_POINT "/ProcessVariablesToLog.txt", "a+");
	}
	
	arc_dsc.width = 3; // Set the width of the arc line
	for (int i = 0; i < 10; i++)
	{
		if (cmd_report_head == cmd_report_tail) break;
		CMD_REPORT_INFO *info = &cmd_report_que[cmd_report_tail];
		if (info->chanel < UI_PLOT_CHANNEL_NUM)
		{
			value.x = info->actrual_freq;
			value.y = info->forward_power;
			if (value.x >= ui_plot_info.min_x && value.x < ui_plot_info.min_x + UI_PLOT_MAX_POINTS)
			{
				ui_plot_scan_points[info->chanel][value.x - ui_plot_info.min_x] =  value.y;	
			}
			if (ui_plot_info.channel_visible[info->chanel])
			{
				pos = ui_plot_val2pos(value);
				arc_dsc.color = lv_color_hex(ui_plot_channel_color[info->chanel]);
				lv_canvas_draw_arc(ui_plot_obj.canvas, pos.x, pos.y, 2, 0, 360, &arc_dsc);
			}
			if (fp)
			{
				sprintf(ui_temp_string,
					"%d,%d,%d,%d,%d,%d,%d,%s\n",
					(int)info->chanel,
					(int)info->actrual_freq,
					(int)info->dac,
					(int)info->forward_power, 
					(int)info->refected_power,
					(int)info->array_plugin,
					(int)info->process_mode,
					info->major_step);
				fwrite(ui_temp_string, strlen(ui_temp_string), 1, fp);
			}
			
		}
		
		cmd_report_tail++;
		cmd_report_tail &= 0xf;	
	}
	if (fp) fclose(fp);
}

void ui_plot_axis(lv_obj_t* parent, bool is_label)
{
	const lv_font_t* font = &lv_font_montserrat_12;
	lv_obj_t* obj;
	lv_draw_line_dsc_t line_dsc;
	lv_draw_label_dsc_t label_dsc;
	lv_draw_line_dsc_init(&line_dsc);
	lv_draw_label_dsc_init(&label_dsc);
	line_dsc.color = lv_color_hex(0x888888); // Set line color
	line_dsc.width = 1; // Set line width
	line_dsc.opa = LV_OPA_60; // Set line opacity (fully opaque)
	label_dsc.color = lv_color_hex(0xFFFFFFF);
	label_dsc.opa = 90;
	label_dsc.font = &lv_font_montserrat_16;
	static char lbl[20] = { 0 };	
	lv_point_t pos, val;
	lv_point_t lines[4];
	lines[0].x = 0; lines[0].y = 0;
	lines[1].x = 0; lines[1].y = ui_plot_height-1;
	lines[2].x = ui_plot_width-1; lines[2].y = ui_plot_height-1;
	lines[3].x = ui_plot_width-1; lines[3].y = 0;
	lv_canvas_fill_bg(ui_plot_obj.canvas, lv_color_black(), LV_OPA_TRANSP);
	lv_canvas_draw_line(ui_plot_obj.canvas, lines, 4, &line_dsc);
	
	for (int i = ui_plot_info.min_x, j = 0; i <= ui_plot_info.max_x; i+= ui_plot_info.step_xn, j ++)
	{
		val.x = i; val.y = 0;
		pos = ui_plot_val2pos(val);
		if (is_label)
		{
			sprintf(lbl, "#FFFFFF %d", i);
			obj = ui_create_label(parent, lbl, font);	
			lv_obj_set_pos(obj, pos.x + 20, ui_plot_height + 2);
		}
		
		lines[0].x = pos.x; lines[0].y = 0;
		lines[1].x = pos.x; lines[1].y = ui_plot_height;
		lv_canvas_draw_line(ui_plot_obj.canvas, lines, 2, &line_dsc);
	}
	if(is_label && obj)	lv_obj_set_x(obj, ui_plot_width+5);
	for (int i = ui_plot_info.min_y, j = 0; i <= ui_plot_info.max_y; i += ui_plot_info.step_yn, j++)
	{
		val.x = 0; val.y = i;
		pos = ui_plot_val2pos(val);
		sprintf(lbl, "#FFFFFF %d", i);	
			
		if (is_label)
		{
			obj = ui_create_label(parent, lbl, font);
			lv_obj_set_pos(obj, 0, pos.y - 10);	
			ui_plot_obj.canvas_label_y[j] = obj;
		}
		else if(ui_plot_obj.canvas_label_y[j])
		{
			obj = ui_plot_obj.canvas_label_y[j];
			lv_label_set_text(obj, lbl);
			lv_obj_set_pos(ui_plot_obj.canvas_label_y[j], 0, pos.y - 10);	
		}
		lines[0].x = 0; lines[0].y = pos.y;
		lines[1].x = ui_plot_width; lines[1].y = pos.y;
		lv_canvas_draw_line(ui_plot_obj.canvas, lines, 2, &line_dsc);
	}
	
	lv_canvas_draw_text(ui_plot_obj.canvas, ui_plot_width - 30, 10, 50, &label_dsc, LV_SYMBOL_MINUS);
	lv_canvas_draw_text(ui_plot_obj.canvas, ui_plot_width - 30, ui_plot_height - 30, 50, &label_dsc, LV_SYMBOL_PLUS);
	if (obj) lv_obj_set_pos(obj, 0, 0);
}

void ui_plot_screen_init(void)
{	
	const lv_font_t* font = &lv_font_montserrat_16;
	ui_plot_screen = ui_create_screen();	
	
	ui_create_pct_title(ui_plot_screen, SCREEN_TUNE);
	
	int button_w = 95;
	int button_h = 45;
	int x = 5, y = 3;
	int gap = 2;
	lv_obj_t* obj = ui_create_button(ui_plot_screen, "SCAN", button_w, button_h, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_SCAN);
	ui_change_button_color(obj, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_plot_obj.btn_scan = obj;
	//
	x += button_w + gap;
	button_w += 5;
	obj = ui_create_button(ui_plot_screen, "ANALYZE", button_w, button_h, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_ANALYZE);
	ui_change_button_color(obj, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_plot_obj.btn_analyze = obj;
	x += button_w + gap;
	button_w -= 15;
	obj = ui_create_button(ui_plot_screen, "Apply", button_w, button_h, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_PEAKS_APPLY);
	ui_change_button_color(obj, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_plot_obj.btn_peak = obj;
	x += button_w + gap;
	//index to next button
	obj = ui_create_button(ui_plot_screen, "SAVE", button_w, button_h, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_SAVE);
	ui_change_button_color(obj, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	//next we reposition the CLEAR button to lower left corner
	
	ui_plot_obj.btn_clear = clear_obj;
	lv_obj_set_pos(clear_obj, 5, SCREEN_HEIGHT - 50);
	//
	button_w = 95;
	x = 5;
	obj = ui_create_button(ui_plot_screen, "STOP", button_w, button_h, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_STOP);
	ui_change_button_color(obj, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
	lv_obj_set_pos(obj, x, y + button_h + gap);
	ui_plot_obj.btn_stop = obj;
	

	
//	y = 110;
//	obj = ui_create_label(ui_plot_screen, "ProgFreqs", font);
//	lv_obj_set_pos(obj, x, y);
	y = 100; //y += 35;
	int txt_w = 60, txt_h = 25;
	obj = lv_textarea_create(ui_plot_screen);
	lv_obj_set_size(obj, txt_w, txt_h); 
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_pos(obj, x, y);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_text(obj, "0");
	ui_plot_obj.txt_ch[0] = obj;
	
	obj = ui_create_button(ui_plot_screen, "", 30, 26, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_CHANNLE_00);
	ui_change_button_color(obj, UI_PLOT_COLOR_CH0, UI_PLOT_COLOR_CH0);
	lv_obj_set_pos(obj, x + txt_w + 2, y);
	ui_plot_obj.button_ch[0] = obj;
	y += txt_h + 2;
	
	obj = lv_textarea_create(ui_plot_screen);
	lv_obj_set_size(obj, txt_w, txt_h); 
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_pos(obj, x, y);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_text(obj, "0");
	ui_plot_obj.txt_ch[1] = obj;
	
	obj = ui_create_button(ui_plot_screen, "", 30, 26, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_CHANNLE_01);
	ui_change_button_color(obj, UI_PLOT_COLOR_CH1, UI_PLOT_COLOR_CH1);
	lv_obj_set_pos(obj, x + txt_w + 2, y);
	ui_plot_obj.button_ch[1] = obj;
	y += txt_h + 2;
	
	obj = lv_textarea_create(ui_plot_screen);
	lv_obj_set_size(obj, txt_w, txt_h); 
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_pos(obj, x, y);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_text(obj, "0");
	ui_plot_obj.txt_ch[2] = obj;
	
	obj = ui_create_button(ui_plot_screen, "", 30, 26, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_CHANNLE_02);
	ui_change_button_color(obj, UI_PLOT_COLOR_CH2, UI_PLOT_COLOR_CH2);	
	lv_obj_set_pos(obj, x + txt_w + 2, y);
	ui_plot_obj.button_ch[2] = obj;
	y += txt_h + 2;
	
	obj = lv_textarea_create(ui_plot_screen);
	lv_obj_set_size(obj, txt_w, txt_h); 
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_pos(obj, x, y);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_text(obj, "0");
	ui_plot_obj.txt_ch[3] = obj;
	
	obj = ui_create_button(ui_plot_screen, "", 30, 26, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_CHANNLE_03);
	ui_change_button_color(obj, UI_PLOT_COLOR_CH3, UI_PLOT_COLOR_CH3);
	lv_obj_set_pos(obj, x + txt_w + 2, y);
	ui_plot_obj.button_ch[3] = obj;
	y += txt_h + 2;
	
	obj = lv_textarea_create(ui_plot_screen);
	lv_obj_set_size(obj, txt_w, txt_h); 
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_pos(obj, x, y);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_text(obj, "0");
	ui_plot_obj.txt_ch[4] = obj;
	
	obj = ui_create_button(ui_plot_screen, "", 30, 26, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_CHANNLE_04);
	ui_change_button_color(obj, UI_PLOT_COLOR_CH4, UI_PLOT_COLOR_CH4);
	lv_obj_set_pos(obj, x + txt_w + 2, y);
	ui_plot_obj.button_ch[4] = obj;
	y += txt_h + 2;
	
	obj = lv_textarea_create(ui_plot_screen);
	lv_obj_set_size(obj, txt_w, txt_h); 
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_pos(obj, x, y);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_text(obj, "0");
	ui_plot_obj.txt_ch[5] = obj;
	
	obj = ui_create_button(ui_plot_screen, "", 30, 26, 2, font, ui_plot_event_button_cb, (void*)UI_PLOT_BTN_CHANNLE_05);
	ui_change_button_color(obj, UI_PLOT_COLOR_CH5, UI_PLOT_COLOR_CH5);
	lv_obj_set_pos(obj, x + txt_w + 2, y);
	ui_plot_obj.button_ch[5] = obj;
	y += txt_h + 2;
	
	lv_obj_t* plot_panel = lv_obj_create(ui_plot_screen);	
	lv_obj_clear_flag(plot_panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); /// Flags
	lv_obj_set_pos(plot_panel, button_w + 10, button_h + 10);
	lv_obj_set_style_radius(plot_panel, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(plot_panel, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(plot_panel, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(plot_panel, lv_color_black(), LV_PART_MAIN);
	int w = SCREEN_WIDTH - button_w - 15;
	int h = SCREEN_HEIGHT - button_h - 15;
	lv_obj_set_size(plot_panel, w, h);
	ui_plot_panel = plot_panel;
	ui_plot_width = w - 40;
	ui_plot_height = h - 30;
	
	lv_obj_t *canvas = lv_canvas_create(plot_panel);
	lv_canvas_set_buffer(canvas, ui_plot_buffer, ui_plot_width, ui_plot_height, LV_IMG_CF_TRUE_COLOR);
	
	lv_obj_set_size(canvas, ui_plot_width, ui_plot_height);
	lv_obj_set_pos(canvas, 40, 0);
	ui_plot_obj.canvas = canvas;
	lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(canvas, ui_event_canvas_cb, LV_EVENT_CLICKED, NULL);
	
	//ui_plot_scan_points = (uint16_t*)malloc(UI_PLOT_MAX_POINTS * UI_PLOT_CHANNEL_NUM * sizeof(uint16_t));
	memset(ui_plot_scan_points, 0, sizeof(uint16_t)*UI_PLOT_CHANNEL_NUM * UI_PLOT_MAX_POINTS);
	ui_plot_axis(plot_panel, true);
	lv_timer_create(ui_plot_update_timer, 200, NULL);
	
	// ui_plot_button_status(false);
	// lv_timer_create(ui_plot_update_timer, 200, NULL);
}

void ui_plot_button_status(bool turning)
{
	if (turning)
	{
		ui_change_button_color(ui_plot_obj.btn_scan, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);	
		ui_change_button_color(ui_plot_obj.btn_stop, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
		ui_change_button_color(ui_plot_obj.btn_peak, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);	
		ui_change_button_color(ui_plot_obj.btn_analyze, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);	
		ui_change_button_color(ui_plot_obj.btn_clear, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);	
	}
	else
	{
		ui_change_button_color(ui_plot_obj.btn_scan, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
		ui_change_button_color(ui_plot_obj.btn_stop, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
		ui_change_button_color(ui_plot_obj.btn_peak, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
		ui_change_button_color(ui_plot_obj.btn_analyze, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
		ui_change_button_color(ui_plot_obj.btn_clear, UI_BUTTON_YELLOW_BG_COLOR, UI_BUTTON_YELLOW_FG_COLOR);
	}
}
void ui_plot_emulate_points()
{
	for (int i = 0; i < UI_PLOT_MAX_POINTS; i++)
	{
		ui_plot_scan_points[0][i] = random_int(10, 1400);
	}
}

void ui_plot_analyze()
{
	//ui_plot_emulate_points();
	if (cmd_tuning) return;
	memset(ui_plot_info.channel_peaks, 0, sizeof(lv_point_t) * UI_PLOT_CHANNEL_NUM);
	for (int i = 0; i < UI_PLOT_CHANNEL_NUM; i++)
	{
		for (int j = 0; j < UI_PLOT_MAX_POINTS; j++)
		{
			uint16_t power = ui_plot_scan_points[i][j];
			if (ui_plot_info.channel_peaks[i].y < power)
			{
				ui_plot_info.channel_peaks[i].y = power;
				ui_plot_info.channel_peaks[i].x = ui_plot_info.min_x + j;
			}
		}
	}
	
	uint16_t max_y = 0;	
	for (int i = 0; i < UI_PLOT_CHANNEL_NUM; i++)
	{
		// ui_plot_info.channel_peaks[i].x = random_int(9000, 10000);
		// ui_plot_info.channel_peaks[i].y = random_int(1000, 2000);
		if (ui_plot_info.channel_peaks[i].y > max_y) max_y = ui_plot_info.channel_peaks[i].y;
		ui_textarea_set_nmuber(ui_plot_obj.txt_ch[i], ui_plot_info.channel_peaks[i].x);
	}
	if (max_y >= 1000)
	{
		ui_plot_info.max_y = ceil((max_y + 100) / 100.0) * 100;
		ui_plot_info.step_yn = ((ui_plot_info.max_y - ui_plot_info.min_y) / 50) * 10;
		ui_plot_refresh_by_buffer();
	}
}

void ui_plot_peaks()
{
	if (cmd_tuning) return;
	lv_point_t pos;
	lv_point_t line[2];
	lv_draw_line_dsc_t line_dsc;
	lv_draw_arc_dsc_t arc_dsc;
	lv_draw_line_dsc_init(&line_dsc);
	lv_draw_arc_dsc_init(&arc_dsc);
	arc_dsc.width = 3; // Set the width of the arc line
	line_dsc.width = 1; // Set line width
	line_dsc.opa = LV_OPA_60; // Set line opacity (fully opaque)
	for (int i = 0; i < UI_PLOT_CHANNEL_NUM; i++)
	{
		line_dsc.color = lv_color_hex(ui_plot_channel_color[i]); // Set line color
		//line_dsc.opa = 90;
		pos = ui_plot_val2pos(ui_plot_info.channel_peaks[i]);
		line[0].x = pos.x;	line[0].y = 0;
		line[1].x = pos.x; line[1].y = ui_plot_height;
		lv_canvas_draw_line(ui_plot_obj.canvas, line,2, &line_dsc);
		arc_dsc.color = line_dsc.color;
		//arc_dsc.opa = 100;
		lv_canvas_draw_arc(ui_plot_obj.canvas, pos.x, pos.y, 4, 0, 360, &arc_dsc);
	}
}

void ui_plot_APPLY_SendFreqTo407()
{	
	sprintf(ui_temp_string, SDCARD_MOUNT_POINT "/TuneHistory.txt");
	
	FILE* fp = fopen(ui_temp_string, "a+");
	sprintf(ui_temp_string, "--------------- APPLY START  ----------------\n");
	if(fp) fwrite(ui_temp_string, 1, strlen(ui_temp_string), fp);
	for (uint8_t i = 0; i < UI_PLOT_CHANNEL_NUM; i++)
	{
		if (ui_plot_info.channel_peaks[i].x >= cmd_start_freq && 
			ui_plot_info.channel_peaks[i].x <= cmd_stop_freq)
		{
			sprintf(ui_temp_string, "M801 I0 T%d A%d\n", i, ui_plot_info.channel_peaks[i].x);
			if (fp) fwrite(ui_temp_string, 1, strlen(ui_temp_string), fp);
			communication_tx_commandline(MasterCommPort, ui_temp_string);
		}
	}
	sprintf(ui_temp_string, "--------------- APPLY END  ----------------\n\n");
	if (fp) {
		fwrite(ui_temp_string, 1, strlen(ui_temp_string), fp);
		fflush(fp);
		fclose(fp);
	}
	ui_plot_save_screenshot();	
	//communication_tx_commandline(MasterCommPort, "M801 I665 T0\n"); send the flash command with the Save buton
}

void ui_plot_refresh_by_buffer()
{
	ui_plot_axis(ui_plot_panel, false);
	
	lv_point_t value, pos;
	lv_draw_arc_dsc_t arc_dsc;
	lv_draw_arc_dsc_init(&arc_dsc);
	
	arc_dsc.width = 3; // Set the width of the arc line
	for (int i = 0; i < UI_PLOT_MAX_POINTS; i++)
	{
		for(int j = 0; j < UI_PLOT_CHANNEL_NUM; j ++)
		{
			if (!ui_plot_info.channel_visible[j]) continue;
			value.y = ui_plot_scan_points[j][i];
			value.x = ui_plot_info.min_x + i;
			if (value.y > ui_plot_info.min_y && value.y < ui_plot_info.max_y)
			{
				pos = ui_plot_val2pos(value);
				arc_dsc.color = lv_color_hex(ui_plot_channel_color[j]);
				lv_canvas_draw_arc(ui_plot_obj.canvas, pos.x, pos.y, 2, 0, 360, &arc_dsc);
			}
			//ui_textarea_set_nmuber(ui_plot_obj.txt_ch[info->chanel], value.y);	
		}
	}
}

bool starts_with(const char *str, const char *prefix) {
	size_t prefix_len = strlen(prefix);
	return strncmp(str, prefix, prefix_len) == 0;
}
void ui_plot_save_screenshot()
{
	struct dirent * dir; // for the directory entries
	if (!systemconfig.sdcard.status)
	{
		ui_show_messagebox(MESSAGEBOX_WARNING, "SD card is not mounted.", 1000);
		return;
	}
	DIR * d = opendir(SDCARD_MOUNT_POINT); // open the path
	if (d == NULL) {
		ui_show_messagebox(MESSAGEBOX_ERROR, "SD card read failed.", 1000);
		return; // if was not able, return
	}
	int index = 0;
	while ((dir = readdir(d)) != NULL) // if we were able to read somehting from the directory
	{
		if (dir->d_type != DT_DIR) // if the type is not directory just print it with blue color
		{
			if (starts_with(dir->d_name, "tune"))
			{
				strncpy(ui_temp_string, dir->d_name + 4, 3);
				int cnt = atoi(ui_temp_string);
				if (index < cnt) index = cnt;
			}
		}
	}
	closedir(d);
	index++;
	sprintf(ui_temp_string, SDCARD_MOUNT_POINT "/tune%03d.bmp", index);
	
	uint32_t size = SCREEN_WIDTH*SCREEN_HEIGHT * sizeof(lv_color_t);
	lv_obj_t* screen = lv_scr_act();
	lv_snapshot_take_to_buf(screen, LV_IMG_CF_TRUE_COLOR, &display_caputure_img_dsc, dispaly_snapshot_buffer565, size);
	
	// display_dump_buffer();
	
	save_bmp(ui_temp_string, (uint16_t*)dispaly_snapshot_buffer565, SCREEN_WIDTH, SCREEN_HEIGHT);
	sprintf(temp_string, "%s saved successfully.", ui_temp_string);
	ui_show_messagebox(MESSAGEBOX_INFO, temp_string, 2000);
}