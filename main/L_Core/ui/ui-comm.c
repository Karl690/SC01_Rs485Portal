#include "ui.h"
#include "ui-comm.h"
#include "main.h"
#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"
#include "L_Core/ui/ui-bluetooth.h"
#include "RevisionHistory.h"
#include "L_Core/bluetooth/ble.h"

lv_obj_t* ui_comm_screen;
bool ui_comm_is_xmit = false;
bool ui_comm_is_rcv = false;
bool ui_comm_is_hex = false;
bool ui_comm_is_ack = false;
bool ui_comm_is_ssd = false;

lv_obj_t* ui_comm_btn_ssd;
lv_obj_t* ui_comm_btn_xmit;
lv_obj_t* ui_comm_btn_rcv;
lv_obj_t* ui_comm_btn_hex;
lv_obj_t* ui_comm_lbl_xmit_indicator;
lv_obj_t* ui_comm_lbl_xmit_count;
lv_obj_t* ui_comm_lbl_rcv_indicator;
lv_obj_t* ui_comm_lbl_rcv_count;
lv_obj_t* ui_comm_display_panel;
lv_obj_t* ui_comm_lbl_rx_acks;
lv_obj_t* ui_comm_lbl_tx_acks;
lv_obj_t* ui_comm_btn_ack;

//char ui_comm_log_buffer[UI_COMM_LOG_MAX_LINE][UI_COMM_LOG_MAX_LINE_CHARS];
uint16_t ui_comm_log_head = 0;
uint16_t ui_comm_log_tail = 0;

void ui_comm_call_event_button(uint8_t code, bool direct)
{
	ui_send_button_event(SCREEN_COMM, code, direct);
	switch (code)
	{
	case UI_COMM_BTN_SSD:
		ui_comm_is_ssd = !ui_comm_is_ssd;
		ui_change_button_color(ui_comm_btn_ssd, ui_comm_is_ssd ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_comm_is_ssd ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		SendDisplayStatusCode(ui_comm_is_ssd);
		break;
	case UI_COMM_BTN_PING:
		SendPing();
		break;
	case UI_COMM_BTN_XMIT:
		ui_comm_is_xmit = !ui_comm_is_xmit;
		ui_change_button_color(ui_comm_btn_xmit, ui_comm_is_xmit ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_comm_is_xmit ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		//ui_comm_add_event((const char*)"ABCDEFGHIJKLMNOPQRSTUVWXYZ\nABCDEFGHIJKLMNOPQRSTUVWXYZ\n", UI_COMM_COLOR_SEND, ui_comm_is_hex);
		break;
	case UI_COMM_BTN_RCV:
		ui_comm_is_rcv = !ui_comm_is_rcv;
		ui_change_button_color(ui_comm_btn_rcv, ui_comm_is_rcv ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_comm_is_rcv ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		break;
	case UI_COMM_BTN_HEX:
		ui_comm_is_hex = !ui_comm_is_hex;
		ui_change_button_color(ui_comm_btn_hex, ui_comm_is_hex ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_comm_is_hex ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		break;
	case UI_COMM_BTN_ACK:
		ui_comm_is_ack = !ui_comm_is_ack;
		ui_change_button_color(ui_comm_btn_ack, ui_comm_is_ack ? UI_BUTTON_ACTIVE_BG_COLOR : UI_BUTTON_DISABLE_BG_COLOR, ui_comm_is_ack ? UI_BUTTON_ACTIVE_FG_COLOR : UI_BUTTON_DISABLE_FG_COLOR);
		break;
	}
}

void ui_comm_event_button_cb(lv_event_t* e)
{
	uint8_t code = (uint8_t)(int)lv_event_get_user_data(e);
	ui_comm_call_event_button(code, true);
}


void ui_comm_update_indicator_timer(lv_timer_t * timer)
{
	// if (!lv_obj_is_visible(ui_comm_screen)) return;
	if (!MasterCommPort) return;

	if (ComUart1.RxIndicator > 0)
	{
		ComUart1.RxIndicator --;
		if (lv_obj_is_visible(ui_comm_screen)) 
			lv_obj_set_style_text_color(ui_comm_lbl_rcv_indicator, lv_color_hex(UI_BUTTON_ACTIVE_FG_COLOR), LV_PART_MAIN); 
	}
	else
	{
		lv_obj_set_style_text_color(ui_comm_lbl_rcv_indicator, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN); 
	}
	if (ComUart1.TxIndicator > 0)
	{
		ComUart1.TxIndicator--;
		if (lv_obj_is_visible(ui_comm_screen)) 
			lv_obj_set_style_text_color(ui_comm_lbl_xmit_indicator, lv_color_hex(UI_BUTTON_ACTIVE_FG_COLOR), LV_PART_MAIN); 
	}
	else
	{
		lv_obj_set_style_text_color(ui_comm_lbl_xmit_indicator, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN); 
	}
	
	ui_label_set_nmuber(ui_comm_lbl_rcv_count, ComUart1.NumberOfCharactersReceived);
	ui_label_set_nmuber(ui_comm_lbl_xmit_count, ComUart1.NumberOfCharactersSent);
	
	ui_label_set_nmuber(ui_comm_lbl_tx_acks, ComUart1.TxAcknowledgeCounter);
	ui_label_set_nmuber(ui_comm_lbl_rx_acks, ComUart1.RxAcknowledgeCounter);
}

void ui_comm_screen_init(void)
{	
	const lv_font_t* font = &lv_font_montserrat_16;
	ui_comm_screen = ui_create_screen();	
	
	
	ui_create_pct_title(ui_comm_screen, SCREEN_COMM);
	
	int x = 10, y = 70;
	int button_w = 60;
	int button_large_width = 90;
	int button_h = 45;
	int gap = 5;
	
	lv_obj_t* obj = ui_create_label(ui_comm_screen, (char*)"SERIAL" MajorStep, &mono_bold_28);	
	lv_obj_set_pos(obj, 230, 7);
	
	gap = 5;
	x = 10, y = 50 + gap;
	obj = ui_create_button(ui_comm_screen, "PING", button_large_width, button_h, 2, font, ui_comm_event_button_cb, (void*)UI_COMM_BTN_PING);
	lv_obj_set_pos(obj, x, y);
	
	x = button_large_width + gap * 5;
	obj = ui_create_label(ui_comm_screen, "TxAck#:", &lv_font_montserrat_16);
	lv_obj_set_pos(obj, x, y+10);
	obj = ui_create_label(ui_comm_screen, "0", &lv_font_montserrat_16);
	lv_obj_set_pos(obj, x + 55, y+10); ui_comm_lbl_tx_acks = obj;
	x += 120;
	obj = ui_create_label(ui_comm_screen, "RxAck#:", &lv_font_montserrat_16);
	lv_obj_set_pos(obj, x, y+10);
	obj = ui_create_label(ui_comm_screen, "0", &lv_font_montserrat_16);
	lv_obj_set_pos(obj, x + 55, y+10); ui_comm_lbl_rx_acks = obj;
	
	
	obj = ui_create_button(ui_comm_screen, "SSD", button_w, button_h, 2, font, ui_comm_event_button_cb, (void*)UI_COMM_BTN_SSD);
	lv_obj_set_pos(obj, SCREEN_WIDTH - 2* (button_w + 5), y); ui_comm_btn_ssd = obj;
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	
	x = SCREEN_WIDTH - button_w - 5;
	obj = ui_create_button(ui_comm_screen, "ACK", button_w, button_h, 2, font, ui_comm_event_button_cb, (void*)UI_COMM_BTN_ACK);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_comm_btn_ack = obj;
	
	y += button_h + gap;
	x = 10;
	
	obj = ui_create_label(ui_comm_screen, LV_SYMBOL_UP, &lv_font_montserrat_20);
	lv_obj_set_style_text_color(obj, lv_color_hex(UI_BUTTON_DISABLE_FG_COLOR), LV_PART_MAIN);	
	lv_obj_set_pos(obj, x, y);
	ui_comm_lbl_xmit_indicator = obj;
	
	obj = ui_create_label(ui_comm_screen, "0", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, x + 30, y + 5);
	lv_obj_set_width(obj, 50);
	lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0);	
	ui_comm_lbl_xmit_count = obj;
	y += 25;
	
	obj = ui_create_button(ui_comm_screen, "XMIT", button_large_width, button_h, 2, font, ui_comm_event_button_cb, (void*)UI_COMM_BTN_XMIT);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_comm_btn_xmit = obj;
	
	y += button_h + gap;
	obj = ui_create_button(ui_comm_screen, "RCV", button_large_width, button_h, 2, font, ui_comm_event_button_cb, (void*)UI_COMM_BTN_RCV);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_comm_btn_rcv = obj;
	
	y += button_h +gap;
	obj = ui_create_label(ui_comm_screen, LV_SYMBOL_DOWN, &lv_font_montserrat_20);
	lv_obj_set_style_text_color(obj, lv_color_hex(UI_BUTTON_DISABLE_FG_COLOR), LV_PART_MAIN);	
	lv_obj_set_pos(obj, x, y);
	ui_comm_lbl_rcv_indicator = obj;
	
	
	obj = ui_create_label(ui_comm_screen, "0", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, x + 30, y);
	lv_obj_set_width(obj, 50);
	lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0);
	ui_comm_lbl_rcv_count = obj;
	y += 25;
	
	obj = ui_create_button(ui_comm_screen, "HEX", button_large_width, button_h, 2, font, ui_comm_event_button_cb, (void*)UI_COMM_BTN_HEX);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_DISABLE_FG_COLOR);
	lv_obj_set_pos(obj, x, y);
	ui_comm_btn_hex = obj;
	

	
	obj = lv_obj_create(ui_comm_screen);
	lv_obj_set_size(obj, SCREEN_WIDTH - button_large_width - gap * 3, SCREEN_HEIGHT - 120);
	lv_obj_set_pos(obj, button_large_width + gap*3, (button_h+ gap)*2 + gap); 
	lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(obj, lv_color_hex(UI_PANEL_BACGROUND_COLOR), LV_PART_MAIN);
	ui_comm_display_panel = obj;
	
	for (uint8_t i = 0; i < UI_LOG_MAX_LINE; i++)
	{
		obj = lv_label_create(ui_comm_display_panel);
		lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP); /*Automatically break long lines*/
		lv_obj_set_style_border_color(obj, lv_color_hex(0x550055), LV_PART_MAIN);
		lv_obj_set_style_text_font(obj, &mono_regualr_20, LV_PART_MAIN);
		lv_obj_set_width(obj, lv_pct(95)); 
		lv_obj_set_x(obj, 5); 
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}

	
	lv_timer_create(ui_comm_update_indicator_timer, 500, NULL);
	
}

char ui_comm_temp_string1[1024] = { 0 };
char ui_comm_temp_string2[256] = { 0 };

void ui_comm_add_event(const char* log, uint32_t color, bool ishex)
{
	if (!log) return;
	uint16_t index = ui_comm_log_head % UI_LOG_MAX_LINE;
	memset(ui_comm_temp_string1, 0, 1024);
	memset(ui_comm_temp_string2, 0, 256);
	lv_obj_t* obj = lv_obj_get_child(ui_comm_display_panel, index);
	lv_obj_set_height(obj, LV_SIZE_CONTENT);
	lv_obj_set_style_text_color(obj, lv_color_hex(color), LV_PART_MAIN);
	if (ishex)
	{
		uint16_t numberofcharcterstoadd = 16;
		uint16_t len =  strlen(log);
		if (numberofcharcterstoadd > len) numberofcharcterstoadd = len;
		int count = 0;
		char* hexString = ui_comm_temp_string1;
		char* asciiString = ui_comm_temp_string2;
		
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
				strcpy(hexString, ui_comm_temp_string2);
				hexString += strlen(ui_comm_temp_string2);
				ui_comm_temp_string2[0] = '\0';
				*hexString = '\n';
				hexString++; 
				asciiString = ui_comm_temp_string2;
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
		strcpy(hexString, ui_comm_temp_string2);
		lv_label_set_text(obj, ui_comm_temp_string1);	
	}
	else
	{
		lv_label_set_text(obj, log);	
	}
	//lv_obj_set_style_bg_color(obj, lv_color_hex(0x550055), LV_PART_MAIN);
	
	ui_comm_log_head++;
	ui_comm_log_tail = ui_comm_log_head < UI_LOG_MAX_LINE ? 0 : ui_comm_log_head - UI_LOG_MAX_LINE;
	
	uint16_t gap = 2;
	uint16_t ypos = 5;
	uint16_t idx = 0;
	uint16_t height = 0;
	for (index = ui_comm_log_tail; index < ui_comm_log_head; index++)
	{
		idx = index % UI_LOG_MAX_LINE;
		obj = lv_obj_get_child(ui_comm_display_panel, idx);
		lv_obj_set_y(obj, ypos);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
		height = lv_obj_get_height(obj);
		ypos += height + gap;
	}
	//lv_obj_scroll_to_y(ui_comm_display_panel, ypos, LV_ANIM_OFF);
}
void ui_comm_add_char(const char code, uint32_t color)
{
	switch (code)
	{
	case PING_CHAR:
		ui_comm_add_event("Send ping 0x7", UI_SEND_COLOR, false);
		break;
	case PING_REPLY:
		ui_comm_add_event("Reply ping 0x6", UI_RECEIVE_COLOR, false);
		break;
	}	
}

void ui_comm_add_log(const char* log, uint32_t color)
{	
	if (ui_comm_display_panel->flags & LV_OBJ_FLAG_HIDDEN) return;
	if ((color == UI_RECEIVE_COLOR && ui_comm_is_rcv) || //incomming data
		(color == UI_SEND_COLOR && ui_comm_is_xmit)) 
		ui_comm_add_event(log, color, ui_comm_is_hex);
}

void ui_comm_clear_log()
{
	ui_comm_log_head = 0;
	ui_comm_log_tail = 0;
	lv_obj_t* obj;
	int count = lv_obj_get_child_cnt(ui_comm_display_panel);
	for (uint8_t i = 0; i < count; i++)
	{
		obj = lv_obj_get_child(ui_comm_display_panel, i);
		lv_label_set_text(obj, "");
		lv_obj_set_x(obj, 5); 
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}
}