#pragma once
#include "lvgl.h"
enum
{
	UI_HOME_BTN_SETTINGS,
	UI_HOME_BTN_SDCARD,
	UI_HOME_BTN_TUNE,
	UI_HOME_BTN_MEG,
	UI_HOME_BTN_BLE,
	UI_HOME_BTN_WIFI,
	UI_HOME_BTN_SECS,
	UI_HOME_BTN_SIMPLE,
};
extern lv_obj_t* ui_home_screen;
void ui_home_screen_init(void);
void ui_home_call_event_button(uint8_t code, bool direct);