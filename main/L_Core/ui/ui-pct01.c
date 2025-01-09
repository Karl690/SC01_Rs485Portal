#include "ui.h"
#include "ui-pct01.h"
#include "main.h"
#include <stdlib.h>
#include "L_Core/ui/ui-comm.h"
#include "K_Core/serial/serial.h"
#include "L_Core/bluetooth/ble.h"
#include "L_Core/ui/ui-bluetooth.h"
//#include "K_core/communication/communication.h"
#include "RevisionHistory.h"

lv_obj_t* ui_pct01_screen;
lv_obj_t* ui_pct01_btn_prog;
lv_obj_t* ui_pct01_xmt_lcd;
lv_obj_t* ui_pct01_forward;
lv_obj_t* ui_pct01_backward;
lv_obj_t* ui_pct01_slider;
lv_obj_t* ui_pct01_slider_label;
lv_obj_t* ui_pct01_keyboard[KEYBOARD_BACK + 1];
lv_obj_t* ui_pct01_label_lines[LCD_LINE_MAX_NUMBER];
bool ui_pct01_is_refresh_line = false;
char ui_pct01_lines[LCD_LINE_MAX_NUMBER][50] = { 0 };
char ui_pct01_temp[256] = { 0 };
uint8_t ui_pct01_diag_indicator = 0;
void ui_pct01_clear_log()
{
	if (!ui_pct01_diag_indicator) return;
	if (run_mode == RUN_BLE_CLIENT) //client => server
		communication_add_string_to_ble_buffer(&bleDevice.TxBuffer, KEYBOARD_CLEAR_STRING);
	else {
		communication_tx_commandline(MasterCommPort, KEYBOARD_CLEAR_STRING);
	}
	
}

void ui_pct01_change_slide_value(uint8_t value)
{
	sprintf(ui_temp_string, "%d", value);
	lv_label_set_text(ui_pct01_slider_label, ui_temp_string);
	sprintf(ui_temp_string, "M623 I%d\n", value);
	communication_add_string_to_serial_buffer(&MasterCommPort->TxBuffer, ui_temp_string);
}
void slider_event_cb(lv_event_t * e)
{
	lv_obj_t * slider = lv_event_get_target(e);
	int value = (int)lv_slider_get_value(slider);
	ui_pct01_change_slide_value(value);
}
void ui_pct01_slider_btn_cb(lv_event_t* e)
{
	uint8_t code = (uint8_t)(int)lv_event_get_user_data(e);
	int value = lv_slider_get_value(ui_pct01_slider);
	switch (code)
	{
	case 0:
		if (value + 1 <= 20) {
			value++;
			lv_slider_set_value(ui_pct01_slider, value, LV_ANIM_ON);
			ui_pct01_change_slide_value(value);
		}
		break;
	case 1:
		if (value - 1 >= 4) {
			value--;
			lv_slider_set_value(ui_pct01_slider, value, LV_ANIM_ON);
			ui_pct01_change_slide_value(value);
		}
		break;
	}
}

void ui_pct01_call_event_button(uint8_t code, bool direct)
{
	ui_send_button_event(SCREEN_MEG_01, code, direct);
	
	ui_pct01_temp[0] = 0;
	ui_pct01_temp[1] = 0;
	switch (code)
	{
	case 0: if (!ui_pct01_diag_indicator) return;
		else ui_pct01_temp[0] = 16; 
		break; //backward LCD
	case 1: if (!ui_pct01_diag_indicator) return;
		ui_pct01_temp[0] = 15; break; ////forward LCD
	case 2:   ui_pct01_temp[0] = 11; break; //XMT LCD 
	}
	if (run_mode == RUN_BLE_CLIENT) //client => server
	communication_add_char_to_ble_buffer(&bleDevice.TxBuffer, ui_pct01_temp[0]);
	else {
		communication_add_char_to_serial_buffer(&MasterCommPort->TxBuffer, ui_pct01_temp[0]);
	}
}
void ui_pct01_event_button_cb(lv_event_t* e)
{
	uint8_t code = (uint8_t)(int)lv_event_get_user_data(e);
	ui_pct01_call_event_button(code, true);
}

void ui_pct01_update_lines_timer(lv_timer_t * timer)
{
	if (ui_pct01_diag_indicator == 0) return;
	ui_pct01_diag_indicator--;
	if (ui_pct01_diag_indicator == 0)
	{
		ui_change_button_color(ui_pct01_forward, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
		ui_change_button_color(ui_pct01_backward, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
		ui_change_button_color(ui_pct01_xmt_lcd, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	}
	else if (ui_pct01_diag_indicator == 3)
	{
		ui_change_button_color(ui_pct01_forward, UI_BUTTON_ACTIVE_BG_COLOR, UI_BUTTON_ACTIVE_FG_COLOR);
		ui_change_button_color(ui_pct01_backward, UI_BUTTON_ACTIVE_BG_COLOR, UI_BUTTON_ACTIVE_FG_COLOR);
		ui_change_button_color(ui_pct01_xmt_lcd, UI_BUTTON_ACTIVE_BG_COLOR, UI_BUTTON_ACTIVE_FG_COLOR);
	}
	 
	if (!ui_pct01_is_refresh_line) return;
	for (int i = 0; i < LCD_LINE_MAX_NUMBER; i++)
		lv_label_set_text(ui_pct01_label_lines[i], ui_pct01_lines[i]);
	ui_pct01_is_refresh_line = false;
}

void ui_pct01_screen_init(void)
{	
	const lv_font_t* font = &lv_font_montserrat_16;
	ui_pct01_screen = ui_create_screen();	
	
	ui_create_pct_title(ui_pct01_screen, SCREEN_MEG_01);
	
	lv_obj_t* obj;
	
	int button_large_width = 130;
	int button_h = 50;
	int gap = 5;
	
	int x = 5, y = 60;
	lv_obj_set_size(clear_obj, button_large_width, button_h);
	lv_obj_set_pos(clear_obj, x, y); y += button_h + gap;
	
	obj = ui_create_button(ui_pct01_screen, "<<BACK", button_large_width, button_h, 2, font, ui_pct01_event_button_cb, (void*)0);
	lv_obj_set_pos(obj, x, y); y += button_h + gap;
	ui_pct01_backward = obj;
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	obj = ui_create_button(ui_pct01_screen, "FORWORD>>", button_large_width, button_h, 2, font, ui_pct01_event_button_cb, (void*)1); 
	lv_obj_set_pos(obj, x, y); y += button_h + gap;
	ui_pct01_forward = obj;
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	
	obj = ui_create_button(ui_pct01_screen, "DIAG LCD", button_large_width, button_h, 2, font, ui_pct01_event_button_cb, (void*)2);
	lv_obj_set_pos(obj, x, y); y += button_h + gap;
	ui_pct01_xmt_lcd = obj;
	ui_change_button_color(ui_pct01_xmt_lcd, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	
	obj = ui_create_label(ui_pct01_screen, "18", &lv_font_montserrat_16);
	lv_obj_set_pos(obj, 160, 10);
	ui_pct01_slider_label = obj;
	
	obj = ui_create_button(ui_pct01_screen, "+", 40, 40, 2, &lv_font_montserrat_24, ui_pct01_slider_btn_cb, 0);
	lv_obj_set_pos(obj, 150, 30);
	
	obj = lv_slider_create(ui_pct01_screen);
	lv_obj_set_size(obj, 30, 170);
	lv_obj_set_pos(obj, 155, 90);
	lv_slider_set_range(obj, 4, 20);
	lv_slider_set_value(obj, 20, LV_ANIM_ON);
	lv_obj_add_event_cb(obj, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
	ui_pct01_slider = obj;
	
	obj = ui_create_button(ui_pct01_screen, "-", 40, 40, 2, &lv_font_montserrat_24, ui_pct01_slider_btn_cb, (void*)1);
	lv_obj_set_pos(obj, 150, 280);
	
	x = 210; y = 10;
	lv_obj_t* panel = lv_obj_create(ui_pct01_screen);
	lv_obj_set_size(panel, SCREEN_WIDTH - x - 2* gap - 80, SCREEN_HEIGHT - 20);
	lv_obj_set_pos(panel, x, y);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE); /// Flags
	lv_obj_set_style_bg_color(panel, lv_color_hex(0), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(panel, 1, LV_PART_MAIN);
	lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
	
	y = 0;
	x = 0;
	for (int i = 0; i < LCD_LINE_MAX_NUMBER; i++)
	{
		obj = ui_create_label(panel, "", &mono_regualr_14); lv_obj_set_pos(obj, x, y); 
		y += 16;	
		ui_pct01_label_lines[i] = obj;
	}
	
	
	lv_timer_create(ui_pct01_update_lines_timer, 500, NULL);
}
void ui_pct01_update_label_text(int index, char* value)
{
	ui_pct01_diag_indicator = 4;
	if (index >= LCD_LINE_MAX_NUMBER) return;
	if (!ui_pct01_label_lines[index]) return;
	
	int len = strlen(value);
	char* line = &ui_pct01_lines[index][0];
	memset(&ui_pct01_lines[index][0], 0, 50); //reset the next buffer
	int c = 0;
	for (int i = 0; i < len; i++)
	{
		if (value[i] == ',')
		{
			index++;
			c = 0;
			memset(&ui_pct01_lines[index][0], 0, 50); //reset the next buffer
			line = &ui_pct01_lines[index][0]; // set the point of next buffer
			continue;
		}
		else if (value[i] == '\n') break;
		else if (c >= 50) break;
		else
		{
			line[c] = value[i];
		}
		c++;
	}
	ui_pct01_is_refresh_line = true;
}
void ui_pct01_update_label_color(int index, char* value)
{
	if (index >= LCD_LINE_MAX_NUMBER) return;
	if (!ui_pct01_label_lines[index]) return;
	uint32_t color = atoi(value);
	lv_obj_set_style_text_color(ui_pct01_label_lines[index], lv_color_hex(color), LV_PART_MAIN);
}
void ui_pct01_update_button_text(int index, char* value)
{
	if (index >= KEYBOARD_HOME) return;
	if (!ui_pct01_keyboard[index]) return;
	lv_obj_t* label = lv_obj_get_child(ui_pct01_keyboard[index], 0);
	if (!label) return;
	lv_label_set_text(label, value);
}
void ui_pct01_update_button_color(int index, char* value)
{
	if (index >= KEYBOARD_HOME) return;
	if (!ui_pct01_keyboard[index]) return;
	uint32_t color = atoi(value);
	ui_change_button_color(ui_pct01_keyboard[index], color, UI_BUTTON_NORMAL_FG_COLOR);
}

void ui_pct01_clear()
{
	ui_pct01_temp[0] = 12; //clear command
	ui_pct01_temp[1] = 0;
	if (run_mode == RUN_BLE_CLIENT) //client => server
		communication_add_char_to_ble_buffer(&bleDevice.TxBuffer, ui_pct01_temp[0]);
	else {
		communication_add_char_to_serial_buffer(&MasterCommPort->TxBuffer, ui_pct01_temp[0]);
	}
	
	for (int i = 0; i < LCD_LINE_MAX_NUMBER; i++)
		lv_label_set_text(ui_pct01_label_lines[i], "");
}