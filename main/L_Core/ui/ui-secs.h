#pragma once
#include "lvgl.h"
extern lv_obj_t* ui_secs_screen;
enum
{	
	UI_SECS_BTN_PARAM,
	UI_SECS_BTN_LOG,
	UI_SECS_BTN_FUNC,
	UI_SECS_BTN_XMIT,
	UI_SECS_BTN_RCV,
	UI_SECS_BTN_HEX,
	UI_SECS_BTN_S1F1W,
	UI_SECS_BTN_S1F5W,
	UI_SECS_BTN_S2F19W,
	UI_SECS_BTN_S2F21W_ON,
	UI_SECS_BTN_S2F21W_OFF,
	UI_SECS_BTN_S7F1W,
	UI_SECS_BTN_S7F3W,
	UI_SECS_BTN_S7F5W,
	UI_SECS_BTN_S9F7,
};

typedef struct
{
	// log screen
	lv_obj_t	*btn_rx, *btn_tx, *btn_hex;
	lv_obj_t	*rx_indicator, *tx_indicator;
	lv_obj_t	*rx_num, *tx_num;
	
	// param screen
	lv_obj_t* setting_reload1timer;
	lv_obj_t* setting_reload2timer;
	lv_obj_t* setting_retrytimer;
	
	lv_obj_t* live_reload1timer;
	lv_obj_t* live_reload2timer;
	lv_obj_t* live_retrytimer;
	
	lv_obj_t* flag;
	lv_obj_t	*rx_calc, *tx_calc;
	lv_obj_t	*rx_secs_num, *tx_secs_num;
	lv_obj_t	*rx_pass, *tx_pass;
	lv_obj_t	*rx_fail, *tx_fail;
	lv_obj_t	*rx_length, *tx_length;
	lv_obj_t	*rx_stm_func, *tx_stm_func;
	lv_obj_t	*rx_sys_bytes, *tx_sys_bytes; 
} UI_SECS_OBJ;

void ui_secs_screen_init(void);
void ui_secs_add_line(const char* log, uint32_t color);
void ui_secs_clear();
void ui_secs_call_event_button(uint8_t code, bool direct);