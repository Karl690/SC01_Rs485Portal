#pragma once
#include "lvgl.h"
extern lv_obj_t* ui_simple_screen;

#define SIMPLE_MAX_ROW_SIZE 40
enum
{	
	UI_SIMPLE_BTN_FUNCS,
	UI_SIMPLE_BTN_XMIT,
	UI_SIMPLE_BTN_RCV,
	UI_SIMPLE_BTN_HEX,
	UI_SIMPLE_BTN_PING,
	UI_SIMPLE_BTN_STARTLOG,
	UI_SIMPLE_BTN_STOPLOG,
	UI_SIMPLE_BTN_S2F21W_ON,
	UI_SIMPLE_BTN_QUERY,
	UI_SIMPLE_BTN_UPDATERECIPE,
	UI_SIMPLE_BTN_DOWNLOADRECIPE,
	UI_SIMPLE_BTN_STARTPROCESS,
	UI_SIMPLE_BTN_STOPPROCESS,
	UI_SIMPLE_BTN_CANCELPROCESS,
	UI_SIMPLE_BTN_IDENTIFYGENERATOR,
	
	UI_SIMPLE_CMD_PING = 50,
	UI_SIMPLE_CMD_START_LOGGING,
	UI_SIMPLE_CMD_STOP_LOGGING,
	UI_SIMPLE_CMD_QUERY,
	UI_SIMPLE_CMD_UPDATE_RECIPE,
	UI_SIMPLE_CMD_DOWNLOAD_RECIPE,
	UI_SIMPLE_CMD_START_PROCESS,
	UI_SIMPLE_CMD_CANCLE_PROCESS,
	UI_SIMPLE_CMD_IDENTIFY,
};

typedef struct
{
	lv_obj_t	*btn_rx, *btn_tx, *btn_hex;
	lv_obj_t	*rx_indicator, *tx_indicator;
	lv_obj_t	*rx_num, *tx_num;
} UI_SIMPLE_OBJ;

extern bool ui_simple_is_rcv;
extern bool ui_simple_is_xmt;
void ui_simple_screen_init(void);
void ui_simple_add_log(const char* log, uint32_t color);
void ui_simple_add_char(const char code, uint32_t color);
void ui_simple_add_line(const char* log, uint32_t color, bool isHex);
void ui_simple_clear();
void ui_simple_call_event_button(uint8_t code, bool direct);