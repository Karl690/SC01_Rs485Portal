#include "ui.h"
#include "ui-comm.h"
#include "main.h"
#include "K_Core/communication/communication.h"
#include "K_Core/simple/simple.h"
#include "L_Core/bluetooth/ble.h"
#include "ui-simple.h"
#include "RevisionHistory.h"

lv_obj_t* ui_simple_screen;
lv_obj_t* ui_simple_log_view;
lv_obj_t* ui_simple_log_panel;
lv_obj_t* ui_simple_func_menu;

bool ui_simple_is_rcv = false;
bool ui_simple_is_xmt = false;
bool ui_simple_is_hex = false;

uint32_t ui_simple_log_head = 0;
uint32_t ui_simple_log_tail = 0;
char ui_simple_temp_string1[1024] = { 0 };
char ui_simple_temp_string2[256] = { 0 };

UI_SIMPLE_OBJ ui_simple_obj;

void ui_simple_update_indicator_timer(lv_timer_t * timer)
{
	if (!simple_obj.serial) return;
	if (systemconfig.serial2.mode != 0) return; // Not Simple mode
	if (simple_obj.serial->RxIndicator > 0)
	{
		simple_obj.serial->RxIndicator--;
		if (lv_obj_is_visible(ui_simple_log_view))
		{	
			lv_obj_set_style_text_color(ui_simple_obj.rx_indicator, 
				lv_color_hex(simple_obj.serial->RxIndicator ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR),
				LV_PART_MAIN);				
		}
		ui_label_set_nmuber(ui_simple_obj.rx_num, simple_obj.serial->NumberOfCharactersReceived);
	}
	
	if (simple_obj.serial->TxIndicator > 0)
	{
		simple_obj.serial->TxIndicator--;
		if (lv_obj_is_visible(ui_simple_log_view))
		{	
			lv_obj_set_style_text_color(ui_simple_obj.tx_indicator, 
				lv_color_hex(simple_obj.serial->TxIndicator  ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR),
				LV_PART_MAIN);				
		}
		ui_label_set_nmuber(ui_simple_obj.tx_num, simple_obj.serial->NumberOfCharactersSent);
	}
}

void ui_simple_call_event_button(uint8_t code, bool direct)
{
	ui_send_button_event(SCREEN_SIMPLE, code, direct);
	switch (code)
	{
	case UI_SIMPLE_BTN_RCV:
		lv_obj_add_flag(ui_simple_func_menu, LV_OBJ_FLAG_HIDDEN);
		ui_simple_is_rcv = !ui_simple_is_rcv;
		ui_change_button_color(ui_simple_obj.btn_rx, ui_simple_is_rcv ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_simple_is_rcv ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		break;
	case UI_SIMPLE_BTN_XMIT:
		lv_obj_add_flag(ui_simple_func_menu, LV_OBJ_FLAG_HIDDEN);
		ui_simple_is_xmt = !ui_simple_is_xmt;
		ui_change_button_color(ui_simple_obj.btn_tx, ui_simple_is_xmt ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_simple_is_xmt ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		break;
	case UI_SIMPLE_BTN_HEX:
		lv_obj_add_flag(ui_simple_func_menu, LV_OBJ_FLAG_HIDDEN);
		ui_simple_is_hex = !ui_simple_is_hex;
		ui_change_button_color(ui_simple_obj.btn_hex, ui_simple_is_hex ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_simple_is_hex ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		break;
	case UI_SIMPLE_BTN_FUNCS:
		if (lv_obj_is_visible(ui_simple_func_menu)) 
		{
			lv_obj_add_flag(ui_simple_func_menu, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_clear_flag(ui_simple_func_menu, LV_OBJ_FLAG_HIDDEN);	
		}
		break;
	default:
		lv_obj_add_flag(ui_simple_func_menu, LV_OBJ_FLAG_HIDDEN);
		if (run_mode != RUN_BLE_CLIENT)
			simple_send_command(code);
		break;
	}
}
void ui_simple_event_button_cb(lv_event_t* e)
{
	int code = (int)lv_event_get_user_data(e);
	ui_simple_call_event_button(code, true);
}

void ui_simple_log_view_init(lv_obj_t* panel)
{
	int x = 0, y = 0;
	int button_w = 60;
	int button_h = 45;
	int gap = 5;
	const lv_font_t* font = &lv_font_montserrat_16;
	lv_obj_t* obj = ui_create_label(panel, LV_SYMBOL_UP, &lv_font_montserrat_20);
	lv_obj_set_style_text_color(obj, lv_color_hex(UI_BUTTON_DISABLE_FG_COLOR), LV_PART_MAIN);	
	lv_obj_set_pos(obj, x, y);
	ui_simple_obj.tx_indicator = obj;
	
	obj = ui_create_label(panel, "0", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, x + 10, y + 5);
	lv_obj_set_width(obj, 50);
	lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0);	
	ui_simple_obj.tx_num = obj;
	y += 25;
	
	obj = ui_create_button(panel, "XMT", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_BTN_XMIT);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_simple_obj.btn_tx = obj;
	
	y += button_h + gap;
	obj = ui_create_button(panel, "RCV", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_BTN_RCV);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_simple_obj.btn_rx = obj;
	
	y += button_h + gap;
	obj = ui_create_label(panel, LV_SYMBOL_DOWN, &lv_font_montserrat_20);
	lv_obj_set_style_text_color(obj, lv_color_hex(UI_BUTTON_DISABLE_FG_COLOR), LV_PART_MAIN);
	lv_obj_set_pos(obj, x, y);
	ui_simple_obj.rx_indicator = obj;
	
	obj = ui_create_label(panel, "0", &lv_font_montserrat_12);
	lv_obj_set_pos(obj, x + 10, y);
	lv_obj_set_width(obj, 50);
	lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0);
	ui_simple_obj.rx_num = obj;
	
	y += 25;
	obj = ui_create_button(panel, "HEX", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_BTN_HEX);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_simple_obj.btn_hex = obj;
	
	obj = lv_obj_create(panel);
	lv_obj_set_size(obj, SCREEN_WIDTH - button_w - gap * 3, SCREEN_HEIGHT - 70);
	lv_obj_set_pos(obj, button_w + gap*2, 0); 
	lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
	//lv_obj_set_style_bg_color(obj, lv_color_hex(UI_PANEL_BACGROUND_COLOR), LV_PART_MAIN);
	ui_simple_log_panel = obj;
	
	for (uint8_t i = 0; i < UI_LOG_MAX_LINE; i++)
	{
		obj = lv_label_create(ui_simple_log_panel);
		lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP); /*Automatically break long lines*/
		lv_obj_set_style_border_color(obj, lv_color_hex(0x550055), LV_PART_MAIN);
		lv_obj_set_style_text_font(obj, &mono_regualr_16, LV_PART_MAIN);
		lv_obj_set_width(obj, lv_pct(95)); 
		lv_obj_set_x(obj, 5); 
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}
	// // lv_timer_create(ui_simple_update_timer, 10, NULL);
	lv_timer_create(ui_simple_update_indicator_timer, 100, NULL);
}
void ui_simple_func_menu_init(lv_obj_t* parent)
{
	lv_obj_t* panel = lv_obj_create(parent);
	uint32_t bg_color = 0x222222;
	uint32_t sh_color = 0x666666;
	uint32_t fg_color = 0xeeeeee;
	//lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); /// Flags
	lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN); /// Flags
	lv_obj_set_size(panel, 210, 220); //480-105
	lv_obj_set_pos(panel, SCREEN_WIDTH - 220, 55); 
	lv_obj_set_style_shadow_color(panel, lv_color_hex(sh_color), LV_PART_MAIN);
	lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(panel, 2, LV_PART_MAIN);
	lv_obj_set_style_bg_color(panel, lv_color_hex(bg_color), LV_PART_MAIN);
	ui_simple_func_menu = panel;
	
	int x = 0, y = 0;
	int button_w = 200;
	int button_h = 55;
	int gap = 3;
	const lv_font_t* font = &lv_font_montserrat_16;
	
	lv_obj_t* obj = ui_create_button(panel, "PING", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_CMD_PING);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "START LOGGING", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_CMD_START_LOGGING);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "STOP LOGGING", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_CMD_STOP_LOGGING);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "QUERY", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_CMD_QUERY);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "UPLOAD RECIPE", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_CMD_UPDATE_RECIPE);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "DOWNLOAD RECIPE", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_CMD_DOWNLOAD_RECIPE);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "Identify Generator", button_w, button_h, 2, font, ui_simple_event_button_cb, (void*)UI_SIMPLE_CMD_IDENTIFY);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
}
void ui_simple_screen_init(void)
{
	//const lv_font_t* font = &lv_font_montserrat_16;
	ui_simple_screen = ui_create_screen();	
	ui_create_pct_title(ui_simple_screen, SCREEN_SIMPLE);
	
	uint8_t tab_width = 80;
	uint8_t tab_x = 225;
	lv_obj_t* obj;
	
	obj = ui_create_button(ui_simple_screen, "FUNCS", tab_width, 50, 2, &lv_font_montserrat_16, ui_simple_event_button_cb, (void*)UI_SIMPLE_BTN_FUNCS);
	lv_obj_set_pos(obj, tab_x + tab_width * 2 +4, 2);
	
	lv_obj_t* panel = lv_obj_create(ui_simple_screen);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); /// Flags
	lv_obj_set_size(panel, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 5 - 60); //480-105
	lv_obj_set_pos(panel, 2, 60); 
	lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(panel, 2, LV_PART_MAIN);
	lv_obj_set_style_bg_color(panel, lv_color_black(), LV_PART_MAIN);
	ui_simple_log_view = panel;
	ui_simple_log_view_init(panel);
	
	ui_simple_func_menu_init(ui_simple_screen);
}


void ui_simple_add_line(const char* log, uint32_t color, bool isHex) {
	uint16_t index = ui_simple_log_head % UI_LOG_MAX_LINE;
	memset(ui_simple_temp_string1, 0, 1024);
	memset(ui_simple_temp_string2, 0, 256);
	lv_obj_t* obj = lv_obj_get_child(ui_simple_log_panel, index);
	lv_obj_set_height(obj, LV_SIZE_CONTENT);
	lv_obj_set_style_text_color(obj, lv_color_hex(color), LV_PART_MAIN);
	if (isHex)
	{
		uint16_t numberofcharcterstoadd = 16;
		uint16_t len =  strlen(log);
		if (numberofcharcterstoadd > len) numberofcharcterstoadd = len;
		int count = 0;
		char* hexString = ui_simple_temp_string1;
		char* asciiString = ui_simple_temp_string2;
		
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
				strcpy(hexString, ui_simple_temp_string2);
				hexString += strlen(ui_simple_temp_string2);
				ui_simple_temp_string2[0] = '\0';
				*hexString = '\n';
				hexString++; 
				asciiString = ui_simple_temp_string2;
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
		strcpy(hexString, ui_simple_temp_string2);
		lv_label_set_text(obj, ui_simple_temp_string1);	
	}
	else
	{
		lv_label_set_text(obj, log);
	}
	//lv_obj_set_style_bg_color(obj, lv_color_hex(0x550055), LV_PART_MAIN);
	
	ui_simple_log_head++;
	ui_simple_log_tail = ui_simple_log_head < UI_LOG_MAX_LINE ? 0 : ui_simple_log_head - UI_LOG_MAX_LINE;
	
	uint16_t gap = 2;
	uint16_t ypos = 5;
	uint16_t idx = 0;
	uint16_t height = 0;
	for (index = ui_simple_log_tail; index < ui_simple_log_head; index++)
	{
		idx = index % UI_LOG_MAX_LINE;
		obj = lv_obj_get_child(ui_simple_log_panel, idx);
		lv_obj_set_y(obj, ypos);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
		height = lv_obj_get_height(obj);
		ypos += height + gap;
	}
	lv_obj_scroll_to_y(ui_simple_log_panel, ypos, LV_ANIM_OFF);
}

void ui_simple_add_char(const char code, uint32_t color)
{
	switch (code)
	{
	case PING_CHAR:
		ui_simple_add_line("Send ping 0x7", UI_SEND_COLOR, false);
		break;
	case PING_REPLY:
		ui_simple_add_line("Reply ping 0x6", UI_RECEIVE_COLOR, false);
		break;
	}	
}
void ui_simple_add_log(const char* log, uint32_t color)
{
	if (!log) return;
	if (!lv_obj_is_visible(ui_simple_log_panel)) return;
	if ((color == UI_RECEIVE_COLOR && ui_simple_is_rcv) || //incomming data
		(color == UI_SEND_COLOR && ui_simple_is_xmt))
	{
		ui_simple_add_line(log, color, ui_simple_is_hex);
	}
}

void ui_simple_clear()
{
	ui_simple_log_head = 0;
	ui_simple_log_tail = 0;
	lv_obj_t* obj;
	lv_obj_add_flag(ui_simple_func_menu, LV_OBJ_FLAG_HIDDEN);
	int count = lv_obj_get_child_cnt(ui_simple_log_panel);
	for (uint8_t i = 0; i < count; i++)
	{
		obj = lv_obj_get_child(ui_simple_log_panel, i); 
		lv_label_set_text(obj, "");
		lv_obj_set_x(obj, 5); 
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}
}

