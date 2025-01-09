#include "ui.h"
#include "ui-control.h"
lv_obj_t* ui_control_screen;

void ui_control_button_handler(lv_event_t * e) {
//	lv_event_code_t event_code = lv_event_get_code(e); 
//	lv_obj_t * target = lv_event_get_target(e);	
}

void ui_control_screen_init()
{
	ui_control_screen = ui_create_screen();	
	ui_create_pct_title(ui_control_screen, SCREEN_CONTROLS);
	
	lv_obj_t* title_label = lv_label_create(ui_control_screen);	
	lv_obj_set_width(title_label, LV_SIZE_CONTENT);
	lv_obj_set_height(title_label, LV_SIZE_CONTENT);
	lv_label_set_recolor(title_label, true);
	lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_label_set_text(title_label, "CONTROL");
	lv_obj_set_style_text_font(title_label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);	
	lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 5);
	
	lv_obj_t* panel = lv_obj_create(ui_control_screen);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE); /// Flags
	lv_obj_set_size(panel, 480, 320-55);
	lv_obj_set_style_bg_color(panel, lv_color_hex(0x0), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_pos(panel, 0, 55);
	
	
}