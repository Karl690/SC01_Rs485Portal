#pragma once
#include "lvgl.h"
#include "font/font.h"

#define SCREEN_WIDTH				480
#define SCREEN_HEIGHT				320
#define SCREEN_PIXEL_SIZE			2
#define SCREEN_LINE_PIXCELS			SCREEN_WIDTH * SCREEN_PIXEL_SIZE

#define UI_BACKGROUND_COLOR			0x0

#define MENU_BACKGROUND_COLOR		0xffffff

#define TITLEBAR_BACKGROUND_COLOR	0x0B72AB
#define TITLEBAR_FORE_COLOR			0xFFFFFF
#define TITLEBAR_HEIGHT				30
#define PAGE_BACKGROUND_COLOR		0x222222
#define BUTTON_BACKGROUND_COLOR		0x0B72AB

#define INFO_COLOR					0x284700
#define WARNING_COLOR				0xEEBA34
#define ERROR_COLOR					0xEE3455

#define UI_PANEL_BACGROUND_COLOR		0x111111
#define UI_BUTTON_NORMAL_BG_COLOR		0x0A1F00
#define UI_BUTTON_NORMAL_FG_COLOR		0xFFFFFF
#define UI_BUTTON_ACTIVE_BG_COLOR		0x406F03
#define UI_BUTTON_ACTIVE_FG_COLOR		0xFFFFFF
#define UI_BUTTON_DISABLE_BG_COLOR		0x383838
#define UI_BUTTON_DISABLE_FG_COLOR		0x888888

#define UI_BUTTON_YELLOW_BG_COLOR		0xffff00
#define UI_BUTTON_YELLOW_FG_COLOR		0x000000

#define UI_MENU_ACTIVE_ITEM_COLOR	0xF30505
#define UI_MENU_NORMAL_ITEM_COLOR	0x494949
#define UI_TEXTAREA_BORDER_COLOR	0x5DA105

#define UI_CHECK_ACTIVE_COLOR		0xF30505
#define UI_CHECK_NONACTIVE_COLOR	0x494949	

#define SETTINGS_LINE_SPACE 45

typedef enum
{
	MESSAGEBOX_WARNING,
	MESSAGEBOX_INFO,
	MESSAGEBOX_ERROR,
}MESSAGEBOX_TYPE;

enum KEYCODE
{
	KEYBOARD_0,
	KEYBOARD_1,
	KEYBOARD_2,
	KEYBOARD_3,
	KEYBOARD_4,
	KEYBOARD_5,
	KEYBOARD_6,
	KEYBOARD_7,
	KEYBOARD_8,
	KEYBOARD_9,
	KEYBOARD_CLEAR,
	KEYBOARD_BACKSPACE,
	KEYBOARD_POINT,
	KEYBOARD_PROG,
	KEYBOARD_ENTER,
	KEYBOARD_START,
	KEYBOARD_STOP,
	KEYBOARD_DIAG,
	KEYBOARD_COMM,
	KEYBOARD_HOME,
	KEYBOARD_BACK,	
	KEYBOARD_SHIFT_ADDRESS,
	KEYBOARD_RESET_ADDRESS,
};
typedef enum
{
	UI_BTN_HOME = 100,
	UI_BTN_CLEAR,
	UI_BTN_SWAP,
	UI_BTN_FN,
}UI_BUTTON_TYPE;
typedef enum
{
	SCREEN_HOME,
	SCREEN_SETTINGS,
	SCREEN_CONTROLS,
	SCREEN_QUALITY,
	SCREEN_SDCARD,
	SCREEN_MEG,
	SCREEN_SECS,
	SCREEN_MEG_01,
	SCREEN_TUNE,
	SCREEN_COMM,
	SCREEN_BLUETOOTH,
	SCREEN_WIFI,
	SCREEN_SIMPLE,
}SCREEN_TYPE;



#define UI_PCT_PANEL_BG_COLOR			0x0A1F00
#define UI_PCT_PANEL_BORDER_COLOR		0x194C01
#define UI_PCT_PANEL_FG_COLOR			0x6DFF13
#define UI_PCT_PANEL_DIAG_BORDER_COLOR	0x621801
#define UI_PCT_PANEL_DIAG_BG_COLOR		0x300000
#define UI_PCT_PANEL_DIAG_FG_COLOR		0xFF0000

#define UI_PCT_BTN_BG_NORMAL_COLOR				0x0A1F00
#define UI_PCT_BTN_BG_ACTIVE_COLOR				0x164102
#define UI_PCT_BTN_FG_NORMAL_COLOR				0x6DFF13
#define UI_PCT_BTN_FG_DISALBE_COLOR				0x717171

#define UI_LOG_MAX_LINE 20
#define UI_LOG_MAX_LINE_CHARS 80

#define UI_SEND_COLOR			0xFF0000
#define UI_RECEIVE_COLOR		0xFFFFFF


#define KEYBOARD_0_STRING "G6 B0\n"
#define KEYBOARD_1_STRING "G6 B1\n"
#define KEYBOARD_2_STRING "G6 B2\n"
#define KEYBOARD_3_STRING "G6 B3\n"
#define KEYBOARD_4_STRING "G6 B4\n"
#define KEYBOARD_5_STRING "G6 B5\n"
#define KEYBOARD_6_STRING "G6 B6\n"
#define KEYBOARD_7_STRING "G6 B7\n"
#define KEYBOARD_8_STRING "G6 B8\n"
#define KEYBOARD_9_STRING "G6 B9\n"
#define KEYBOARD_CLEAR_STRING "G6 B10\n"
#define KEYBOARD_BACKSPACE_STRING "G6 B11\n"
#define KEYBOARD_POINT_STRING "G6 B12\n"
#define KEYBOARD_PROG_STRING "G6 B13\n"
#define KEYBOARD_ENTER_STRING "G6 B14\n"
#define KEYBOARD_START_STRING "G6 B15\n"
#define KEYBOARD_STOP_STRING "G6 B16\n"
#define KEYBOARD_DIAG_STRING "G6 B17\n"
#define KEYBOARD_COMM_STRING "G6 B18\n"
#define KEYBOARD_HOME_STRING "G6 B19\n"
#define KEYBOARD_BACK_STRING "G6 B20\n"
#define KEYBOARD_SHFIT_ADDRESS_STRING "G6 B21\n"
#define KEYBOARD_RESET_ADDRESS_STRING "G6 B22\n"


extern char ui_temp_string[];
extern uint8_t ui_initialized;
extern lv_obj_t * keyboard;
extern lv_obj_t* clear_obj;

extern bool ui_request_update;
extern uint8_t ui_request_screen_id, ui_request_button_id;

void InitUI(void);

lv_obj_t* ui_create_screen();
lv_obj_t* ui_create_titlebar(lv_obj_t* screen, uint32_t color);
lv_obj_t* ui_create_label(lv_obj_t* parent, const char* text, const lv_font_t* font);
lv_obj_t* ui_create_button(lv_obj_t* parent,
	const char* text,
	uint16_t w,
	uint16_t h, 
	uint16_t radius,
	const lv_font_t* font,
	lv_event_cb_t event_button_handler,
	void* event_data);
void ui_change_button_text(lv_obj_t* button, const char* text);
void ui_change_button_color(lv_obj_t* button, uint32_t bg, uint32_t fg);
lv_obj_t* ui_create_panel(lv_obj_t* parent, uint32_t color, bool scrollable);
void ui_transform_screen(uint8_t screen);
void ui_show_messagebox(MESSAGEBOX_TYPE type, const char* msg, uint16_t delay);
void ui_event_go_home_cb(lv_event_t* e);
void ui_event_edit_cb(lv_event_t* e);
void ui_create_pct_title(lv_obj_t* parent, uint8_t screen);
void ui_event_title_button_cb(lv_event_t* e);


void ui_textarea_set_nmuber(lv_obj_t* obj, int number);
int ui_textarea_get_nmuber(lv_obj_t* obj);
void ui_textarea_set_readonly(lv_obj_t* obj, bool b);

void ui_label_set_nmuber(lv_obj_t* obj, int number);
int ui_label_get_nmuber(lv_obj_t* obj);
void ui_call_button_event(uint8_t screen_id, uint8_t button_id, bool);
void ui_send_button_event(uint8_t screen_id, uint8_t button_id, bool);