#pragma once
#include "lvgl.h"

#define LCD_LINE_MAX_NUMBER 18

extern lv_obj_t* ui_pct01_screen;

void ui_pct01_screen_init(void);

void ui_pct01_clear_log();
void ui_pct01_update_label_text(int index, char* value);
void ui_pct01_update_label_color(int index, char* value);
void ui_pct01_update_button_text(int index, char* value);
void ui_pct01_update_button_color(int index, char* value);
void ui_pct01_clear();
void ui_pct01_call_event_button(uint8_t code, bool direct);