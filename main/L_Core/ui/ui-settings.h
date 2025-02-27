#pragma once


typedef enum
{
	SETTINGS_SUBMENU_BLUETOOTH,
	SETTINGS_SUBMENU_WIFI,
	SETTINGS_SUBMENU_OPC,
	SETTINGS_SUBMENU_SERIAL1,
	SETTINGS_SUBMENU_SERIAL2,
	SETTINGS_SUBMENU_SECS,
	SETTINGS_SUBMENU_SDCARD,
	SETTINGS_SUBMENU_SYSTEM,
}SETTINGS_SUBMENU_TYPE;

typedef struct
{
	lv_obj_t* ssid;
	lv_obj_t* password;
	lv_obj_t* status;
	lv_obj_t* autoconnect;
	lv_obj_t* ip;
	lv_obj_t* subnet;
} UI_WIFI;

typedef struct
{
	lv_obj_t* status;
	lv_obj_t* autostart;
	lv_obj_t* screen_control;
} UI_BLUETOOTH;

typedef struct
{
	lv_obj_t* ui_rx_pin;
	lv_obj_t* ui_tx_pin;
	lv_obj_t* ui_baud;
	lv_obj_t* ui_mode;
	lv_obj_t* ui_485;
	lv_obj_t* ui_485_tx;
	lv_obj_t* ui_485_rx;
	lv_obj_t* ui_485_baud;
} UI_SERIAL;

typedef struct
{
	lv_obj_t* ui_timer_reload1;
	lv_obj_t* ui_timer_reload2;
	lv_obj_t* ui_timer_retry;
} UI_SECS;
typedef struct
{
	UI_WIFI ui_wifi;
	UI_BLUETOOTH ui_bluetooth;
	UI_SERIAL ui_serial1;
	UI_SERIAL ui_serial2;
	UI_SECS ui_secs;
}UI_SETTINGS;

extern lv_obj_t* ui_settings_screen;
extern UI_SETTINGS ui_settings;

void ui_settings_screen_init();
void ui_settings_update_configuratiion();
void ui_settings_serial_485_visible(bool enable);