#include "ui.h"
#include "ui-comm.h"
#include "main.h"
#include "ui-secs.h"
#include "K_Core/secs/secs.h"
#include "K_Core/secs/secshelper.h"
#include "L_Core/ui/ui-bluetooth.h"
#include "RevisionHistory.h"
#include "L_Core/bluetooth/ble.h"

lv_obj_t* ui_secs_screen;
bool ui_is_param_view = false; 

lv_obj_t* ui_secs_param_btn;
lv_obj_t* ui_secs_log_btn;

lv_obj_t* ui_secs_param_view;
lv_obj_t* ui_secs_log_view;
lv_obj_t* ui_secs_log_panel;
lv_obj_t* ui_secs_func_menu;


uint8_t ui_secs_tx_indicator;
uint8_t ui_secs_rx_indicator;
bool ui_secs_is_rcv = false;
bool ui_secs_is_xmt = false;
bool ui_secs_is_hex = false;

uint32_t ui_secs_log_head = 0;
uint32_t ui_secs_log_tail = 0;
char ui_secs_temp_string1[1024] = { 0 };
char ui_secs_temp_string2[256] = { 0 };

UI_SECS_OBJ ui_secs_obj;

void ui_secs_add_secs_string_list(char* list, int rows, uint32_t color)
{
	for (int i = 0; i < rows; i++)
	{
		char* line = list + (i * SECS_MAX_ROW_SIZE);
		ui_secs_add_line(line, color);
	}
}
char ui_secs_temp[60] = { 0 };
void ui_secs_add_secs_hex_list(uint8_t* buf, uint32_t color)
{
	uint16_t len = buf[0];
	memset(ui_secs_temp, 0, 60);
	int idx = 0;
	for (int i = 0; i < len; i++)
	{
		if (i % 16 == 0 && i > 0)
		{
			idx = 0;
			ui_secs_add_line(ui_secs_temp, color);
			memset(ui_secs_temp, 0, 60);	
		}
		sprintf(ui_secs_temp + idx, "%02X ", buf[i]);
		idx+=3;
	}
	ui_secs_add_line(ui_secs_temp, color);
}

void ui_secs_update_indicator_timer(lv_timer_t * timer)
{
	if (!secs_obj.serial) return;
	if (systemconfig.serial2.mode != SERIAL2_MODE_SECS) return; // Not Secs mode
	if (secs_obj.serial->RxIndicator > 0)
	{
		secs_obj.serial->RxIndicator--;
		if (lv_obj_is_visible(ui_secs_log_view))
		{	
			lv_obj_set_style_text_color(ui_secs_obj.rx_indicator, 
				lv_color_hex(secs_obj.serial->RxIndicator ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR),
				LV_PART_MAIN);				
		}
	}
	
	if (secs_obj.serial->TxIndicator > 0)
	{
		secs_obj.serial->TxIndicator--;
		if (lv_obj_is_visible(ui_secs_log_view))
		{	
			lv_obj_set_style_text_color(ui_secs_obj.tx_indicator, 
				lv_color_hex(secs_obj.serial->TxIndicator ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR),
				LV_PART_MAIN);				
		}
	}
}

void ui_secs_update_timer(lv_timer_t * timer)
{
	if (!secs_obj.serial) return;
	if (!lv_obj_is_visible(ui_secs_screen)) return;
	int rows = 0;
	if (secs_is_recevied)
	{
		if (ui_secs_is_rcv)
		{
			// parse the secs string from Secs buffer
			rows = ConvertSecsBinaryToStringList(secs_receive_buffer, secsstringReceiveList[0]); //convert the secs buffer to string list.
			ui_secs_add_secs_string_list(secsstringReceiveList[0], rows, UI_RECEIVE_COLOR);	
			if (ui_secs_is_hex)
			{
				ui_secs_add_secs_hex_list(secs_receive_buffer, UI_RECEIVE_COLOR);	
			}
		}
		ui_secs_rx_indicator = 5;
		secs_is_recevied = false;
	}
	if (secs_is_transfered)
	{
		if (ui_secs_is_xmt)
		{
			// parse the secs string from Secs buffer
			rows = ConvertSecsBinaryToStringList(secs_transmit_buffer, secsstringSendList[0]); //convert the secs buffer to string list.
			ui_secs_add_secs_string_list(secsstringSendList[0], rows, UI_SEND_COLOR);	
			if (ui_secs_is_hex)
			{
				ui_secs_add_secs_hex_list(secs_transmit_buffer, UI_SEND_COLOR);
			}
		}
		ui_secs_tx_indicator = 5;
		secs_is_transfered = false;
	}
	
	
	ui_textarea_set_nmuber(ui_secs_obj.live_reload1timer, secstimer1);
	ui_textarea_set_nmuber(ui_secs_obj.live_reload2timer, secstimer2);
	ui_textarea_set_nmuber(ui_secs_obj.live_retrytimer, numberofretriesleft);
	ui_textarea_set_nmuber(ui_secs_obj.rx_calc, CheckedSumReceived);
	ui_textarea_set_nmuber(ui_secs_obj.rx_fail, SecsReceivedMessageTotalErrorNum);
	ui_textarea_set_nmuber(ui_secs_obj.tx_fail, secssendfail);
	ui_textarea_set_nmuber(ui_secs_obj.rx_secs_num, secs_rx_num);
	ui_textarea_set_nmuber(ui_secs_obj.tx_secs_num, secs_tx_num);
	ui_label_set_nmuber(ui_secs_obj.rx_num, secs_rx_num);
	ui_label_set_nmuber(ui_secs_obj.tx_num, secs_tx_num);
	lv_textarea_set_text(ui_secs_obj.rx_stm_func, ReceivedSecsCmd);
	lv_textarea_set_text(ui_secs_obj.tx_stm_func, SentSecsCmd);
	ui_textarea_set_nmuber(ui_secs_obj.flag, secs1_flag);
}

void ui_secs_call_event_button(uint8_t code, bool direct)
{
	ui_send_button_event(SCREEN_SECS, code, direct);
	switch (code)
	{
	case UI_SECS_BTN_PARAM:
		lv_obj_set_style_bg_color(ui_secs_param_btn, lv_color_hex(UI_BUTTON_ACTIVE_BG_COLOR), LV_PART_MAIN); 
		lv_obj_set_style_bg_color(ui_secs_log_btn, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN);
		lv_obj_clear_flag(ui_secs_param_view, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(ui_secs_log_view, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		break;
	case UI_SECS_BTN_LOG:
		lv_obj_set_style_bg_color(ui_secs_param_btn, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN);
		lv_obj_set_style_bg_color(ui_secs_log_btn, lv_color_hex(UI_BUTTON_ACTIVE_BG_COLOR), LV_PART_MAIN); 
		lv_obj_add_flag(ui_secs_param_view, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(ui_secs_log_view, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		break;
	case UI_SECS_BTN_FUNC:
		if (lv_obj_is_visible(ui_secs_func_menu))
			lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		else 
			lv_obj_clear_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		break;
	case UI_SECS_BTN_RCV:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		ui_secs_is_rcv = !ui_secs_is_rcv;
		ui_change_button_color(ui_secs_obj.btn_rx, ui_secs_is_rcv ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_secs_is_rcv ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		break;
	case UI_SECS_BTN_XMIT:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		ui_secs_is_xmt = !ui_secs_is_xmt;
		ui_change_button_color(ui_secs_obj.btn_tx, ui_secs_is_xmt ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_secs_is_xmt ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		break;
	case UI_SECS_BTN_HEX:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		ui_secs_is_hex = !ui_secs_is_hex;
		ui_change_button_color(ui_secs_obj.btn_hex, ui_secs_is_hex ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_secs_is_hex ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		break;
	case UI_SECS_BTN_S1F1W:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		SendSecsCommand(s1f1message, sizeof(s1f1message));
		break;
	case UI_SECS_BTN_S1F5W:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		SendSecsCommand(s1f5message, sizeof(s1f5message));
		break;
	case UI_SECS_BTN_S2F19W:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		VerteqSRD_idme(s2f19message);
		SendSecsCommand(s2f19message, sizeof(s2f19message));
		break;
	case UI_SECS_BTN_S2F21W_ON:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		s7f1message[13] = 1;
		SendSecsCommand(s2f21message, sizeof(s2f21message));
		break;
	case UI_SECS_BTN_S2F21W_OFF:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		s7f1message[13] = 0x1b;
		SendSecsCommand(s2f21message, sizeof(s2f21message));
		break;
	case UI_SECS_BTN_S7F1W:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		SendSecsCommand(s7f1message, sizeof(s7f1message));
		break;
	case UI_SECS_BTN_S7F3W:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		SendSecsCommand(s7f3message, sizeof(s7f3message));
		break;
	case UI_SECS_BTN_S7F5W:
		lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
		SendSecsCommand(s7f5message, sizeof(s7f5message));
		break;
	}
}

void ui_secs_event_button_cb(lv_event_t* e)
{
	int code = (int)lv_event_get_user_data(e);
	ui_secs_call_event_button(code, true);
}

void ui_secs_param_view_init(lv_obj_t* panel)
{
	int y = 25;
	int x = 5;
	int width = 50;
	int height = 35;
	
	lv_obj_t* obj = lv_textarea_create(panel);
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_pos(obj, x, y);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_nmuber(obj, systemconfig.secs.timerReload1);
	ui_secs_obj.setting_reload1timer = obj;
	
	obj = ui_create_label(panel, "<Time 1>", &lv_font_montserrat_16);
	lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 55, y + 10);
	
	obj = lv_textarea_create(panel);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 130, y);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_readonly(obj, true);
	ui_secs_obj.live_reload1timer = obj;
	
	y += 40;
	obj = lv_textarea_create(panel);
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_pos(obj, x, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_nmuber(obj, systemconfig.secs.timerReload2);
	ui_secs_obj.setting_reload2timer = obj;
	
	obj = ui_create_label(panel, "<Time 2>", &lv_font_montserrat_16);
	lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 55, y + 10);
	
	obj = lv_textarea_create(panel);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 130, y);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_readonly(obj, true);
	ui_secs_obj.live_reload2timer = obj;
	
	y += 40;
	obj = lv_textarea_create(panel);
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_pos(obj, x, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_textarea_set_nmuber(obj, systemconfig.secs.timerRetry);
	ui_secs_obj.setting_retrytimer = obj;
	
	obj = ui_create_label(panel, "<Time 3>", &lv_font_montserrat_16);
	lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 55, y + 10);
	
	obj = lv_textarea_create(panel);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 130, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.live_retrytimer = obj;
	
	y += 40;
	obj = ui_create_label(panel, " Flag ", &lv_font_montserrat_16);
	lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 55, y + 10);
	
	
	y += 40;
	obj = lv_textarea_create(panel);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 55, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.flag = obj;
	
	obj = ui_create_label(panel, LV_SYMBOL_DOWN " RCV", &lv_font_montserrat_16);
	lv_obj_set_pos(obj, 210, 0);
	obj = ui_create_label(panel, LV_SYMBOL_UP " XMT", &lv_font_montserrat_16);
	lv_obj_set_pos(obj, 390, 0);
	
	lv_obj_t* sub_panel = lv_obj_create(panel);
	lv_obj_set_size(sub_panel, SCREEN_WIDTH - 205, SCREEN_HEIGHT - 5 - 90); //480-105
	lv_obj_set_pos(sub_panel, 190, 25); 
	lv_obj_set_style_border_width(sub_panel, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(sub_panel, 2, LV_PART_MAIN);
	lv_obj_set_style_bg_color(sub_panel, lv_color_black(), LV_PART_MAIN);
	
	y = 0; x = 0;
	width = 80;
	obj = lv_textarea_create(sub_panel);
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_pos(obj, x, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.rx_calc = obj;
	
	obj = ui_create_label(sub_panel, " CALC ", &lv_font_montserrat_16);
	lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 85, y + 10);
	
	obj = lv_textarea_create(sub_panel);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 185, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.tx_calc = obj;
	
	
	y += 40;
	obj = lv_textarea_create(sub_panel);
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_pos(obj, x, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.rx_num = obj;
	
	obj = ui_create_label(sub_panel, " rcv/xmt ", &lv_font_montserrat_16);
	lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 85, y + 10);
	
	obj = lv_textarea_create(sub_panel);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 185, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.tx_num = obj;
	
	y += 40;
	obj = lv_textarea_create(sub_panel);
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_pos(obj, x, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.rx_fail = obj;
	
	obj = ui_create_label(sub_panel, " Fail N", &lv_font_montserrat_16);
	lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 85, y + 10);
	
	obj = lv_textarea_create(sub_panel);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 185, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.tx_fail = obj;

	
	y += 40;
	obj = lv_textarea_create(sub_panel);
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_pos(obj, x, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.rx_stm_func = obj;
	
	obj = ui_create_label(sub_panel, " Strm/Fnct ", &lv_font_montserrat_16);
	lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 85, y + 10);
	
	obj = lv_textarea_create(sub_panel);
	lv_obj_set_size(obj, width, height); 
	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 185, y);
	ui_textarea_set_readonly(obj, true);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
	ui_secs_obj.tx_stm_func = obj;
	
	
//	y += 40;
//	obj = lv_textarea_create(sub_panel);
//	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
//	lv_obj_set_size(obj, width, height); 
//	lv_obj_set_pos(obj, x, y);
//	ui_textarea_set_readonly(obj, true);
//	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
//	ui_secs_obj.rx_sys_bytes = obj;
//	
//	obj = ui_create_label(sub_panel, " Sys Bytes ", &lv_font_montserrat_16);
//	lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN);
//	lv_obj_set_pos(obj, x + 85, y + 10);
//	
//	obj = lv_textarea_create(sub_panel);
//	lv_obj_set_size(obj, width, height); 
//	lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN);
//	lv_obj_set_pos(obj, x + 185, y);
//	ui_textarea_set_readonly(obj, true);
//	lv_textarea_set_align(obj, LV_TEXT_ALIGN_CENTER);
//	ui_secs_obj.tx_sys_bytes = obj;
}
void ui_secs_log_view_init(lv_obj_t* panel)
{
	int x = 0, y = 0;
	int button_w = 60;
	int button_h = 45;
	int gap = 5;
	const lv_font_t* font = &lv_font_montserrat_16;
	lv_obj_t* obj = ui_create_label(panel, LV_SYMBOL_UP, &lv_font_montserrat_20);
	lv_obj_set_style_text_color(obj, lv_color_hex(UI_BUTTON_DISABLE_FG_COLOR), LV_PART_MAIN);	
	lv_obj_set_pos(obj, x, y);
	ui_secs_obj.tx_indicator = obj;
	
	obj = ui_create_label(panel, "0", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, x + 10, y + 5);
	lv_obj_set_width(obj, 50);
	lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0);	
	ui_secs_obj.tx_num = obj;
	y += 25;
	
	obj = ui_create_button(panel, "XMT", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_XMIT);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_secs_obj.btn_tx = obj;
	
	y += button_h + gap;
	obj = ui_create_button(panel, "RCV", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_RCV);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_secs_obj.btn_rx = obj;
	
	y += button_h + gap;
	obj = ui_create_label(panel, LV_SYMBOL_DOWN, &lv_font_montserrat_20);
	lv_obj_set_style_text_color(obj, lv_color_hex(UI_BUTTON_DISABLE_FG_COLOR), LV_PART_MAIN);	
	lv_obj_set_pos(obj, x, y);
	ui_secs_obj.rx_indicator = obj;
	
	obj = ui_create_label(panel, "0", &lv_font_montserrat_12);
	lv_obj_set_pos(obj, x + 10, y);
	lv_obj_set_width(obj, 50);
	lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0);
	ui_secs_obj.rx_num = obj;
	
	y += 25;
	obj = ui_create_button(panel, "HEX", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_HEX);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_secs_obj.btn_hex = obj;
	
	obj = lv_obj_create(panel);
	lv_obj_set_size(obj, SCREEN_WIDTH - button_w - gap * 3, SCREEN_HEIGHT - 70);
	lv_obj_set_pos(obj, button_w + gap*2, 0); 
	lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
	//lv_obj_set_style_bg_color(obj, lv_color_hex(UI_PANEL_BACGROUND_COLOR), LV_PART_MAIN);
	ui_secs_log_panel = obj;
	
	for (uint8_t i = 0; i < UI_LOG_MAX_LINE; i++)
	{
		obj = lv_label_create(ui_secs_log_panel);
		lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP); /*Automatically break long lines*/
		lv_obj_set_style_border_color(obj, lv_color_hex(0x550055), LV_PART_MAIN);
		lv_obj_set_style_text_font(obj, &mono_regualr_16, LV_PART_MAIN);
		lv_obj_set_width(obj, lv_pct(95)); 
		lv_obj_set_x(obj, 5); 
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}
	lv_timer_create(ui_secs_update_timer, 10, NULL);
	lv_timer_create(ui_secs_update_indicator_timer, 100, NULL);
}
void ui_secs_func_menu_init(lv_obj_t* parent)
{
	uint32_t bg_color = 0x222222;
	uint32_t sh_color = 0x666666;
	uint32_t fg_color = 0xeeeeee;
	lv_obj_t* panel = lv_obj_create(parent);
	lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN); /// Flags
	lv_obj_set_size(panel, 150, 220); //480-105
	lv_obj_set_pos(panel, SCREEN_WIDTH - 160, 55);
	lv_obj_set_style_shadow_color(panel, lv_color_hex(sh_color), LV_PART_MAIN);
	lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(panel, 2, LV_PART_MAIN);
	lv_obj_set_style_bg_color(panel, lv_color_hex(bg_color), LV_PART_MAIN);
	ui_secs_func_menu = panel;
	int x = 0, y = 0;
	int button_w = 140;
	int button_h = 55;
	int gap = 3;
	const lv_font_t* font = &lv_font_montserrat_16;

	lv_obj_t* obj = ui_create_button(panel, "S1F1W", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_S1F1W);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "S1F5W", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_S1F5W);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "S2F19W", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_S2F19W);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "S2F21W_ON", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_S2F21W_ON);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "S2F21W_OFF", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_S2F21W_ON);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "S7F1W", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_S7F1W);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "S7F3W", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_S7F3W);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "S7F5W", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_S7F5W);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
	
	y += button_h + gap;
	obj = ui_create_button(panel, "S9F7", button_w, button_h, 2, font, ui_secs_event_button_cb, (void*)UI_SECS_BTN_S9F7);
	ui_change_button_color(obj, bg_color, fg_color);
	lv_obj_set_pos(obj, x, y);
}

void ui_secs_screen_cb(lv_event_t* e)
{
	//lv_obj_add_flag(ui_secs_func_menu, LV_OBJ_FLAG_HIDDEN);
}
void ui_secs_screen_init(void)
{	
	//const lv_font_t* font = &lv_font_montserrat_16;
	ui_secs_screen = ui_create_screen();	
	lv_obj_add_flag(ui_secs_screen, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(ui_secs_screen, ui_secs_screen_cb, LV_EVENT_CLICKED, NULL);
	ui_create_pct_title(ui_secs_screen, SCREEN_SECS);
	
	uint8_t tab_width = 80;
//	lv_obj_t* obj = ui_create_label(ui_secs_screen, (char*)"SECS", &mono_bold_28);	
//	lv_obj_set_pos(obj, 217, 7);
//	
	uint8_t tab_x = 225;
	lv_obj_t* obj = ui_create_button(ui_secs_screen, "PARAM", tab_width, 50, 2, &lv_font_montserrat_16, ui_secs_event_button_cb, (void*)UI_SECS_BTN_PARAM);
	lv_obj_set_pos(obj, tab_x, 2);
	ui_secs_param_btn = obj;
	
	obj = ui_create_button(ui_secs_screen, "LOG", tab_width, 50, 2, &lv_font_montserrat_16, ui_secs_event_button_cb, (void*)UI_SECS_BTN_LOG);
	lv_obj_set_pos(obj, tab_x + tab_width * 1+2, 2);
	lv_obj_set_style_bg_color(obj, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN);
	ui_secs_log_btn = obj;
	
	obj = ui_create_button(ui_secs_screen, "FUNCS", tab_width, 50, 2, &lv_font_montserrat_16, ui_secs_event_button_cb, (void*)UI_SECS_BTN_FUNC);
	lv_obj_set_pos(obj, tab_x + tab_width * 2 +4, 2);
	
	
	lv_obj_t* panel = lv_obj_create(ui_secs_screen);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); /// Flags
	lv_obj_set_size(panel, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 5 - 60); //480-105
	lv_obj_set_pos(panel, 2, 60); 
	lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(panel, 2, LV_PART_MAIN);
	lv_obj_set_style_bg_color(panel, lv_color_black(), LV_PART_MAIN);
	ui_secs_param_view = panel;
	ui_secs_param_view_init(panel);
	
	panel = lv_obj_create(ui_secs_screen);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE); /// Flags
	lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN); /// Flags
	lv_obj_set_size(panel, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 5 - 60); //480-105
	lv_obj_set_pos(panel, 2, 60); 
	lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(panel, 2, LV_PART_MAIN);
	lv_obj_set_style_bg_color(panel, lv_color_black(), LV_PART_MAIN);
	ui_secs_log_view = panel;
	ui_secs_log_view_init(panel);
	
	
	ui_secs_func_menu_init(ui_secs_screen);
	
	lv_obj_set_style_bg_color(ui_secs_param_btn, lv_color_hex(UI_BUTTON_ACTIVE_BG_COLOR), LV_PART_MAIN); 
	lv_obj_set_style_bg_color(ui_secs_log_btn, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN);
	lv_obj_clear_flag(ui_secs_param_view, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(ui_secs_log_view, LV_OBJ_FLAG_HIDDEN);
}
void ui_secs_add_line(const char* log, uint32_t color)
{
	if (!log) return;
	uint16_t index = ui_secs_log_head % UI_LOG_MAX_LINE;
	memset(ui_secs_temp_string1, 0, 1024);
	memset(ui_secs_temp_string2, 0, 256);
	lv_obj_t* obj = lv_obj_get_child(ui_secs_log_panel, index);
	lv_obj_set_height(obj, LV_SIZE_CONTENT);
	lv_obj_set_style_text_color(obj, lv_color_hex(color), LV_PART_MAIN);
//	if (ishex)
//	{
//		uint16_t numberofcharcterstoadd = 16;
//		uint16_t len =  strlen(log);
//		if (numberofcharcterstoadd > len) numberofcharcterstoadd = len;
//		int count = 0;
//		char* hexString = ui_secs_temp_string1;
//		char* asciiString = ui_secs_temp_string2;
//		
//		while (count < len)
//		{
//			sprintf(hexString, "%02X ", log[count]); hexString += 3;
//			
//			if ((log[count] >= ' ') & (log[count] <= 127))
//			{
//				*asciiString = (char)(log[count]);
//			}
//			else
//			{
//				*asciiString = '.';
//			}
//			asciiString++;
//			count++;
//			if ((count & 7) == 0)//we just got to 8 characters so display them and go to next line
//			{
//				*hexString = ' '; hexString++;
//				strcpy(hexString, ui_secs_temp_string2);
//				hexString += strlen(ui_secs_temp_string2);
//				ui_secs_temp_string2[0] = '\0';
//				*hexString = '\n';
//				hexString++; 
//				asciiString = ui_secs_temp_string2;
//			}
//		}
//		int fill = (3 * (8 - (count & 7))); //lets see how many characters we need
//		for (count = 0; count < fill - 1; count++)
//		{ 
//			*hexString = '-';
//			hexString++;
//		}
//		*hexString = ' '; hexString++;
//		*hexString = ' '; hexString++;
//		strcpy(hexString, ui_secs_temp_string2);
//		lv_label_set_text(obj, ui_secs_temp_string1);	
//	}
//	else
	{
		lv_label_set_text(obj, log);
	}
	//lv_obj_set_style_bg_color(obj, lv_color_hex(0x550055), LV_PART_MAIN);
	
	ui_secs_log_head++;
	ui_secs_log_tail = ui_secs_log_head < UI_LOG_MAX_LINE ? 0 : ui_secs_log_head - UI_LOG_MAX_LINE;
	
	uint16_t gap = 2;
	uint16_t ypos = 5;
	uint16_t idx = 0;
	uint16_t height = 0;
	for (index = ui_secs_log_tail; index < ui_secs_log_head; index++)
	{
		idx = index % UI_LOG_MAX_LINE;
		obj = lv_obj_get_child(ui_secs_log_panel, idx);
		lv_obj_set_y(obj, ypos);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
		height = lv_obj_get_height(obj);
		ypos += height + gap;
	}
	lv_obj_scroll_to_y(ui_secs_log_panel, ypos, LV_ANIM_OFF);
}

void ui_secs_clear()
{
	ui_secs_log_head = 0;
	ui_secs_log_tail = 0;
	lv_obj_t* obj;
	int count = lv_obj_get_child_cnt(ui_secs_log_panel);
	for (uint8_t i = 0; i < count; i++)
	{
		obj = lv_obj_get_child(ui_secs_log_panel, i);
		lv_label_set_text(obj, "");
		lv_obj_set_x(obj, 5); 
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}
}

