#pragma once
#include "lvgl.h"

enum
{	
	UI_COMM_BTN_SSD,
	UI_COMM_BTN_PING,
	UI_COMM_BTN_XMIT,
	UI_COMM_BTN_RCV,
	UI_COMM_BTN_HEX,
	UI_COMM_BTN_ACK,
};

extern lv_obj_t* ui_comm_screen;

extern bool ui_comm_is_ack;
extern bool ui_comm_is_hex;
void ui_comm_screen_init(void);
void ui_comm_add_event(const char* log, uint32_t color, bool ishex);
void ui_comm_add_log(const char* log, uint32_t color);
void ui_comm_add_char(const char code, uint32_t color);
void ui_comm_clear_log();
void ui_come_call_event_button(uint8_t code, bool direct);