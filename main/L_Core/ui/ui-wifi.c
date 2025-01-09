#include "ui.h"
#include "ui-control.h"
lv_obj_t* ui_wifi_screen;

void ui_wifi_screen_init()
{
	ui_wifi_screen = ui_create_screen();	
	ui_create_pct_title(ui_wifi_screen, SCREEN_WIFI);
	
	lv_obj_t* title_label = lv_label_create(ui_wifi_screen);	
	lv_obj_set_width(title_label, LV_SIZE_CONTENT);
	lv_obj_set_height(title_label, LV_SIZE_CONTENT);
	lv_label_set_recolor(title_label, true);
	lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_label_set_text(title_label, "WIFI");
	lv_obj_set_style_text_font(title_label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);	
	lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 5);
	
	lv_obj_t* panel = lv_obj_create(ui_wifi_screen);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE); /// Flags
	lv_obj_set_size(panel, 480, 320-55);
	lv_obj_set_style_bg_color(panel, lv_color_hex(0x0), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_pos(panel, 0, 55);
	
	lv_obj_t* obj = ui_create_label(panel,
		"WIFI FEATURE NOT AVAILABLE!\nCheck with factory for Firmware upgrade.", &lv_font_montserrat_20);
	lv_obj_set_style_text_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
	
}