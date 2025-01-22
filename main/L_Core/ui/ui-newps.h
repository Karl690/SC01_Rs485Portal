#pragma once
#include "lvgl.h"
extern lv_obj_t* ui_newps_screen;
enum
{
	UI_NEWPS_BTN_PING,
	UI_NEWPS_BTN_PWRON,
	UI_NEWPS_BTN_PWROFF,
	UI_NEWPS_BTN_INC_200,
	UI_NEWPS_BTN_DEC_200
};
typedef struct
{
	lv_obj_t* ping;
	lv_obj_t* pwron;
	lv_obj_t* pwroff;
	lv_obj_t* v_plus200;
	lv_obj_t* v_minus200;
	lv_obj_t* log_panel;
	lv_obj_t* prg;
	lv_obj_t* act;
	lv_obj_t* checksum;
	lv_obj_t* slider;
	lv_obj_t* slider_label;
	lv_obj_t* emulator;
} UI_NEWPS_OBJ;

void ui_newps_screen_init(void);
void ui_newps_add_line(const char* log, uint32_t color);
void ui_newps_call_event_button(uint8_t code, bool direct);
void ui_newps_change_slide_value(int value);

void ui_newps_add_event(const char* log, uint32_t color, bool ishex);
void ui_newps_add_char(const char code, uint32_t color);
void ui_newps_add_log(const char* log, uint32_t color);
void ui_newps_add_log(const char* log, uint32_t color);
void ui_newps_add_command(uint8_t* buff, size_t len, bool inOut);
void ui_newps_clear_log();