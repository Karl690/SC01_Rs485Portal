#include "ui.h"
#include "ui-newps.h"
#include "main.h"
#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"
#include "L_Core/ui/ui-bluetooth.h"
#include "RevisionHistory.h"
#include "L_Core/bluetooth/ble.h"
#include "K_Core/supply/supply.h"

lv_obj_t* ui_newps_screen;
uint16_t ui_newps_log_head = 0;
uint16_t ui_newps_log_tail = 0;
UI_NEWPS_OBJ ui_newps_obj;
void ui_newps_call_event_button(uint8_t code, bool direct)
{
	ui_send_button_event(SCREEN_COMM, code, direct);
	switch (code)
	{
	case UI_NEWPS_BTN_PING:
		supply_read_teslaman_status(); //SendPing();
		break;
	case UI_NEWPS_BTN_PWROFF:
		supply_turn_off_voltage();
		break;
	case UI_NEWPS_BTN_PWRON:
		supply_turn_on_voltage();
		break;
	case UI_NEWPS_BTN_INC_200:
		if (supply_status_info.prog_voltage + 200 <= SUPPLY_MAX_VOLTAGE) {
			supply_status_info.prog_voltage += 200;
			supply_set_teslaman_voltage_current(supply_status_info.prog_voltage + 200, 1);
		}
		break;
	case UI_NEWPS_BTN_DEC_200:
		if (supply_status_info.prog_voltage - 200 >= SUPPLY_MIN_VOLTAGE)
		{
			supply_status_info.prog_voltage -= 200;
			supply_set_teslaman_voltage_current(supply_status_info.prog_voltage, 1);
		}
		break;
	}
}

void ui_newps_event_button_cb(lv_event_t* e)
{
	uint8_t code = (uint8_t)(int)lv_event_get_user_data(e);
	ui_newps_call_event_button(code, true);
}


void ui_newps_update_indicator_timer(lv_timer_t * timer)
{
	sprintf(ui_temp_string, "%d", supply_status_info.prog_voltage / 1000);
	lv_label_set_text(ui_newps_obj.slider_label, ui_temp_string);
	sprintf(ui_temp_string, "PRG V=%.02fkV", supply_status_info.prog_voltage / 1000.0);
	lv_label_set_text(ui_newps_obj.prg, ui_temp_string);
	
	sprintf(ui_temp_string, "ACT V=%.02fkV", supply_status_info.actual_voltage / 1000.0);
	lv_label_set_text(ui_newps_obj.act, ui_temp_string);
	
	sprintf(ui_temp_string, "CKSUM=%X", supply_checksum);
	lv_label_set_text(ui_newps_obj.checksum, ui_temp_string);
}


void ui_newps_change_slide_value(int value)
{
	supply_status_info.prog_voltage = value;
	supply_set_teslaman_voltage_current(value, 1);
}
void ui_newps_slider_event_cb(lv_event_t * e)
{
	lv_obj_t * slider = lv_event_get_target(e);
	int value = (int)lv_slider_get_value(slider);
	ui_newps_change_slide_value(value * 1000);
}
void ui_newps_slider_btn_cb(lv_event_t* e)
{
	uint8_t code = (uint8_t)(int)lv_event_get_user_data(e);
	int value = lv_slider_get_value(ui_newps_obj.slider);
	switch (code)
	{
	case 0:
		if (value + 1 <= SUPPLY_MAX_VOLTAGE / 1000) {
			value+=1;
			lv_slider_set_value(ui_newps_obj.slider, value, LV_ANIM_ON);
			ui_newps_change_slide_value(value * 1000);
		}
		break;
	case 1:
		if (value - 1 >= 0) {
			value-=1;
			lv_slider_set_value(ui_newps_obj.slider, value, LV_ANIM_ON);
			ui_newps_change_slide_value(value * 1000);
		}
		break;
	}
}


void ui_newps_screen_init(void)
{	
	const lv_font_t* font = &lv_font_montserrat_16;
	ui_newps_screen = ui_create_screen();	
	
	
	ui_create_pct_title(ui_newps_screen, SCREEN_COMM);
	
	int x = 10, y = 70;
	int button_large_width = 90;
	int button_h = 45;
	int gap = 5;
	
	lv_obj_t* obj = ui_create_label(ui_newps_screen, (char*)"SERIAL" MajorStep, &mono_bold_28);	
	lv_obj_set_pos(obj, 230, 7);
	
	gap = 5;
	x = 10, y = 50 + gap;
	obj = ui_create_button(ui_newps_screen, "PING", button_large_width, button_h, 2, font, ui_newps_event_button_cb, (void*)UI_NEWPS_BTN_PING);
	ui_newps_obj.ping = obj;
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(ui_newps_screen, "PWR ON", button_large_width, button_h, 2, font, ui_newps_event_button_cb, (void*)UI_NEWPS_BTN_PWRON);
	// ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	ui_newps_obj.pwron= obj;
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(ui_newps_screen, "PWR OFF", button_large_width, button_h, 2, font, ui_newps_event_button_cb, (void*)UI_NEWPS_BTN_PWROFF);
	// ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	ui_newps_obj.pwroff = obj;
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(ui_newps_screen, "V+=200", button_large_width, button_h, 2, font, ui_newps_event_button_cb, (void*)UI_NEWPS_BTN_INC_200);
	// ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	ui_newps_obj.v_plus200 = obj;
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(ui_newps_screen, "V-=200", button_large_width, button_h, 2, font, ui_newps_event_button_cb, (void*)UI_NEWPS_BTN_DEC_200);
	// ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	ui_newps_obj.v_minus200 = obj;
	lv_obj_set_pos(obj, x, y);
	
	x = button_large_width + gap * 3;
	y = 50 + gap;
	obj = ui_create_label(ui_newps_screen, "PRG V=0.00Kv", &lv_font_montserrat_14);
	lv_obj_set_style_text_color(obj, lv_color_hex(COLOR_YELLOW), LV_PART_MAIN);
	ui_newps_obj.prg = obj;
	lv_obj_set_pos(obj, x, y);
	y += 25;
	obj = ui_create_label(ui_newps_screen, "ACT V=0.00Kv", &lv_font_montserrat_14);
	lv_obj_set_style_text_color(obj, lv_color_hex(COLOR_YELLOW), LV_PART_MAIN);
	ui_newps_obj.act = obj;
	lv_obj_set_pos(obj, x, y);
	
	obj = ui_create_label(ui_newps_screen, "CKSUM=FFFF", &lv_font_montserrat_14);
	lv_obj_set_style_text_color(obj, lv_color_hex(COLOR_WHITE), LV_PART_MAIN);
	ui_newps_obj.checksum = obj;
	lv_obj_set_pos(obj, x+220, y-25);
	
	obj = lv_obj_create(ui_newps_screen);
	lv_obj_set_size(obj, SCREEN_WIDTH - button_large_width - gap * 3 - 50, SCREEN_HEIGHT - 120);
	lv_obj_set_pos(obj, x, (button_h + gap) * 2 + gap); 
	lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(obj, lv_color_hex(UI_PANEL_BACGROUND_COLOR), LV_PART_MAIN);
	ui_newps_obj.log_panel = obj;
	
	for (uint8_t i = 0; i < UI_LOG_MAX_LINE; i++)
	{
		obj = ui_create_label(ui_newps_obj.log_panel, "", &mono_regualr_14);
		lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP); /*Automatically break long lines*/
		// lv_obj_set_style_border_color(obj, lv_color_hex(0x550055), LV_PART_MAIN);
		// lv_obj_set_style_text_font(obj, &mono_regualr_20, LV_PART_MAIN);
		lv_obj_set_width(obj, lv_pct(95)); 
		lv_obj_set_x(obj, 5); 
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}
	lv_timer_create(ui_newps_update_indicator_timer, 500, NULL);
	
	x = SCREEN_WIDTH - gap - 40;
	y = 50 + gap;
	obj = ui_create_label(ui_newps_screen, "18", &lv_font_montserrat_16);
	lv_obj_set_pos(obj, x+10, y);
	ui_newps_obj.slider_label = obj;
	y += 25;
	obj = ui_create_button(ui_newps_screen, "+", 40, 40, 2, &lv_font_montserrat_24, ui_newps_slider_btn_cb, 0);
	lv_obj_set_pos(obj, x, y);
	y += button_h + gap;
	obj = lv_slider_create(ui_newps_screen);
	lv_obj_set_size(obj, 10, 120);
	lv_obj_set_pos(obj, x+15, y);
	lv_slider_set_range(obj, SUPPLY_MIN_KVOLTAGE, SUPPLY_MAX_KVOLTAGE);
	lv_slider_set_value(obj, 0, LV_ANIM_ON);
	lv_obj_add_event_cb(obj, ui_newps_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
	ui_newps_obj.slider = obj;
	
	y += 130 + gap;
	obj = ui_create_button(ui_newps_screen, "-", 40, 40, 2, &lv_font_montserrat_24, ui_newps_slider_btn_cb, (void*)1);
	lv_obj_set_pos(obj, x, y);
}

char ui_newps_temp_string1[1024] = { 0 };
char ui_newps_temp_string2[256] = { 0 };

void ui_newps_add_event(const char* log, uint32_t color, bool ishex)
{
	if (!log) return;
	uint16_t index = ui_newps_log_head % UI_LOG_MAX_LINE;
	memset(ui_newps_temp_string1, 0, 1024);
	memset(ui_newps_temp_string2, 0, 256);
	lv_obj_t* obj = lv_obj_get_child(ui_newps_obj.log_panel, index);
	lv_obj_set_height(obj, LV_SIZE_CONTENT);
	lv_obj_set_style_text_color(obj, lv_color_hex(color), LV_PART_MAIN);
	if (ishex)
	{
		uint16_t numberofcharcterstoadd = 16;
		uint16_t len =  strlen(log);
		if (numberofcharcterstoadd > len) numberofcharcterstoadd = len;
		int count = 0;
		char* hexString = ui_newps_temp_string1;
		char* asciiString = ui_newps_temp_string2;
		
		while (count < len)
		{
			sprintf(hexString, "%02X ", log[count]); hexString += 3;
			
			if ((log[count] >= ' ') & (log[count] <= 127))
			{
				*asciiString = (char)(log[count]);
			}
			else
			{
				*asciiString = '.';
			}
			asciiString++;
			count++;
			if ((count & 7) == 0)//we just got to 8 characters so display them and go to next line
			{
				*hexString = ' '; hexString++;
				strcpy(hexString, ui_newps_temp_string2);
				hexString += strlen(ui_newps_temp_string2);
				ui_newps_temp_string2[0] = '\0';
				*hexString = '\n';
				hexString++; 
				asciiString = ui_newps_temp_string2;
			}
		}
		int fill = (3 * (8 - (count & 7))); //lets see how many characters we need
		for (count = 0; count < fill - 1; count++)
		{ 
			*hexString = '-';
			hexString++;
		}
		*hexString = ' '; hexString++;
		*hexString = ' '; hexString++;
		strcpy(hexString, ui_newps_temp_string2);
		lv_label_set_text(obj, ui_newps_temp_string1);	
	}
	else
	{
		lv_label_set_text(obj, log);	
	}
	//lv_obj_set_style_bg_color(obj, lv_color_hex(0x550055), LV_PART_MAIN);
	
	ui_newps_log_head++;
	ui_newps_log_tail = ui_newps_log_head < UI_LOG_MAX_LINE ? 0 : ui_newps_log_head - UI_LOG_MAX_LINE;
	
	uint16_t gap = 2;
	uint16_t ypos = 5;
	uint16_t idx = 0;
	uint16_t height = 0;
	for (index = ui_newps_log_tail; index < ui_newps_log_head; index++)
	{
		idx = index % UI_LOG_MAX_LINE;
		obj = lv_obj_get_child(ui_newps_obj.log_panel, idx);
		lv_obj_set_y(obj, ypos);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
		height = lv_obj_get_height(obj);
		ypos += height + gap;
	}
	lv_obj_scroll_to_y(ui_newps_obj.log_panel, ypos, LV_ANIM_OFF);
}
void ui_newps_add_char(const char code, uint32_t color)
{
	switch (code)
	{
	case PING_CHAR:
		ui_newps_add_event("Send ping 0x7", UI_SEND_COLOR, false);
		break;
	case PING_REPLY:
		ui_newps_add_event("Reply ping 0x6", UI_RECEIVE_COLOR, false);
		break;
	}	
}

void ui_newps_add_log(const char* log, uint32_t color)
{	
	if (ui_newps_obj.log_panel->flags & LV_OBJ_FLAG_HIDDEN) return;
	ui_newps_add_event(log, color, false);
}

void ui_newps_add_command(uint8_t* buff, size_t len, bool inOut)
{
	memset(temp_string, 0, 256);
	temp_string[0] = inOut? '<': '>';
	temp_string[1] = inOut ? '<' : '>';
	temp_string[2] = ' ';
	char* temp = temp_string + 3;
	for (uint8_t i = 0; i < len; i++)
	{
		sprintf(temp, "%02X ", buff[i]);
		temp += 3;
	}
	if (ui_newps_obj.log_panel->flags & LV_OBJ_FLAG_HIDDEN) return;
	ui_newps_add_event(temp_string, inOut ? UI_RECEIVE_COLOR : UI_SEND_COLOR, false);
}
void ui_newps_clear_log()
{
	ui_newps_log_head = 0;
	ui_newps_log_tail = 0;
	lv_obj_t* obj;
	int count = lv_obj_get_child_cnt(ui_newps_obj.log_panel);
	for (uint8_t i = 0; i < count; i++)
	{
		obj = lv_obj_get_child(ui_newps_obj.log_panel, i);
		lv_label_set_text(obj, "");
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}
}