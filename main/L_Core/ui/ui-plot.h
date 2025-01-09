#include "lvgl.h"
#define UI_PLOT_CHANNEL_NUM 6
#define UI_PLOT_MAX_POINTS	1024
enum  
{
	UI_PLOT_BTN_SCAN,  
	UI_PLOT_BTN_ANALYZE,  
	UI_PLOT_BTN_PEAKS_APPLY,  
	UI_PLOT_BTN_STOP,
	UI_PLOT_BTN_SAVE,
	UI_PLOT_BTN_CHANNLE_00,
	UI_PLOT_BTN_CHANNLE_01,
	UI_PLOT_BTN_CHANNLE_02,
	UI_PLOT_BTN_CHANNLE_03,
	UI_PLOT_BTN_CHANNLE_04,
	UI_PLOT_BTN_CHANNLE_05,
	UI_PLOT_BTN_ZOOMIN,
	UI_PLOT_BTN_ZOOMOUT,
};

enum
{
	UI_PLOT_COLOR_CH0 = 0xff0000,
	UI_PLOT_COLOR_CH1 = 0x00ff00,
	UI_PLOT_COLOR_CH2 = 0x0000ff,
	UI_PLOT_COLOR_CH3 = 0xffff00,
	UI_PLOT_COLOR_CH4 = 0x00ffff,
	UI_PLOT_COLOR_CH5 = 0xff00ff,
};

typedef struct
{
	uint16_t min_x;
	uint16_t max_x;
	uint16_t min_y;
	uint16_t max_y;
	uint16_t step_xn;
	uint16_t step_yn;
	uint8_t channel_visible[UI_PLOT_CHANNEL_NUM];
	lv_point_t channel_peaks[UI_PLOT_CHANNEL_NUM];
}UI_PLOT_INFO;

typedef struct
{
	lv_obj_t* btn_scan;
	lv_obj_t* btn_stop;
	lv_obj_t* btn_analyze;
	lv_obj_t* btn_peak;
	lv_obj_t* btn_clear;
	lv_obj_t* button_ch[UI_PLOT_CHANNEL_NUM];
	lv_obj_t* txt_ch[UI_PLOT_CHANNEL_NUM];
	lv_obj_t* canvas;
	lv_obj_t* canvas_label_y[6];
} UI_PLOT_OBJ;
extern lv_obj_t *ui_plot_screen;

void ui_plot_screen_init(void);
void ui_plot_update();
void ui_plot_clear();
void ui_plot_axis(lv_obj_t* parent, bool is_label);
void ui_plot_button_status(bool turning);

void ui_plot_analyze();
void ui_plot_peaks();
void ui_plot_APPLY_SendFreqTo407();
void ui_plot_refresh_by_buffer();
void ui_plot_call_event_button(uint8_t code, bool direct);

void ui_plot_save_screenshot();