#include "main.h"
#include "ui.h"
#include "ui-settings.h"
#include "L_Core/sd-card/sd-card.h"
#include "L_Core/bluetooth/ble.h"
#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"
#include "RevisionHistory.h"
lv_obj_t* ui_settings_screen;
lv_obj_t* ui_settings_bluetooth_page;
lv_obj_t* ui_settings_wifi_page;
lv_obj_t* ui_settings_opc_page;
lv_obj_t* ui_settings_sdcard_page;
lv_obj_t* ui_settings_serial_page1;
lv_obj_t* ui_settings_serial_page2;
lv_obj_t* ui_settings_secs_page;
lv_obj_t* ui_settings_system_page;
lv_obj_t* settings_active_menu = NULL;
lv_obj_t* settings_active_page = NULL;
bool ui_settings_initialized = false;


UI_SETTINGS ui_settings;

int get_value_from_index(int index)
{
	int p = index + 10;
	if (p == 15) p = 21;
	return p;
}
int get_index_from_value(int value)
{
	int i = value - 10;
	if (i > 4) i = 5;
	return i;
}
int get_index_from_baud(int baud)
{
	int index = 0;
	switch (baud)
	{
	case 9600: index = 0; break;
	case 14400: index = 1; break;
	case 19200: index = 2; break;
	case 38400: index = 3; break;
	case 57600: index = 4; break;
	case 115200: index = 5; break;
	case 230400: index = 6; break;
	case 460800: index = 7; break;
	case 921600: index = 8; break;
	case 1000000: index = 9; break;
	}
	return index;
}

int get_baud_from_index(int index)
{
	int baud = 115200;
	switch (index)
	{
	case 0: baud = 9600; break;
	case 1: baud = 14400; break;
	case 2: baud = 19200; break;
	case 3: baud = 38400; break;
	case 4: baud = 57600; break;
	case 5: baud = 115200; break;
	case 6: baud = 230400; break;
	case 7: baud = 460800; break;
	case 8: baud = 921600; break;
	case 9: baud = 1000000; break;
	}
	return baud;
}

void ui_settings_event_submenu_cb(lv_event_t* e)
{
	if (!ui_settings_initialized) return;
	lv_obj_t * target = lv_event_get_target(e);	
	SETTINGS_SUBMENU_TYPE type = (SETTINGS_SUBMENU_TYPE)(int) e->user_data;
	if (settings_active_menu  == target) return;
	if (settings_active_menu) lv_obj_set_style_bg_color(settings_active_menu, lv_color_hex(UI_BUTTON_NORMAL_BG_COLOR), LV_PART_MAIN);
	lv_obj_set_style_bg_color(target, lv_color_hex(UI_BUTTON_ACTIVE_BG_COLOR), LV_PART_MAIN);
	
	if (settings_active_page) lv_obj_add_flag(settings_active_page, LV_OBJ_FLAG_HIDDEN);
	switch (type)
	{
	case SETTINGS_SUBMENU_BLUETOOTH:
		settings_active_page = ui_settings_bluetooth_page;
		break;
	case SETTINGS_SUBMENU_WIFI:
		settings_active_page = ui_settings_wifi_page;
		break;
	case SETTINGS_SUBMENU_OPC:
		settings_active_page = ui_settings_opc_page;
		break;
	case SETTINGS_SUBMENU_SERIAL1:
		settings_active_page = ui_settings_serial_page1;
		break;
	case SETTINGS_SUBMENU_SERIAL2:
		settings_active_page = ui_settings_serial_page2;
		break;
	case SETTINGS_SUBMENU_SECS:
		settings_active_page = ui_settings_secs_page;
		break;
	case SETTINGS_SUBMENU_SDCARD:
		settings_active_page = ui_settings_sdcard_page;
		break;
	case SETTINGS_SUBMENU_SYSTEM:
		settings_active_page = ui_settings_system_page;
		break;
	default:
		break;
	}
	if (settings_active_page) lv_obj_clear_flag(settings_active_page, LV_OBJ_FLAG_HIDDEN);
	settings_active_menu = target;
}

void ui_setttings_serial_test_event_cb(lv_event_t* e)
{
	if (settings_active_page == ui_settings_serial_page1)
	{
		communication_add_string_to_serial_buffer(&ComUart1.TxBuffer, "Hello!");
	}
	else
	{
		communication_add_string_to_serial_buffer(&ComUart2.TxBuffer, "Hello!");
	}
}
void ui_settings_serial_load_event_cb(lv_event_t* e)
{
	UI_SERIAL* ui_serial = settings_active_page == ui_settings_serial_page1 ? &ui_settings.ui_serial1 : &ui_settings.ui_serial2;
	uint8_t* data = (uint8_t*)lv_event_get_user_data(e);
	if (data == 0)
	{
		if (settings_active_page == ui_settings_serial_page1)
		{
			lv_dropdown_set_selected(ui_serial->ui_tx_pin, get_index_from_value(GPIO_NUM_12));
			lv_dropdown_set_selected(ui_serial->ui_rx_pin, get_index_from_value(GPIO_NUM_14));	
		}
		else
		{
			lv_dropdown_set_selected(ui_serial->ui_tx_pin, get_index_from_value(GPIO_NUM_11));
			lv_dropdown_set_selected(ui_serial->ui_rx_pin, get_index_from_value(GPIO_NUM_13));	
		}
		
		lv_dropdown_set_selected(ui_serial->ui_baud, get_index_from_baud(115200));
	}
	else
	{
		if (settings_active_page == ui_settings_serial_page1)
		{
			lv_dropdown_set_selected(ui_serial->ui_tx_pin, get_index_from_value(GPIO_NUM_21));
			lv_dropdown_set_selected(ui_serial->ui_rx_pin, get_index_from_value(GPIO_NUM_14));	
		}
		else
		{
			lv_dropdown_set_selected(ui_serial->ui_tx_pin, get_index_from_value(GPIO_NUM_11));
			lv_dropdown_set_selected(ui_serial->ui_rx_pin, get_index_from_value(GPIO_NUM_13));	
		}
		lv_dropdown_set_selected(ui_serial->ui_baud, get_index_from_baud(115200));
	}
	
}
void ui_settings_event_switch_cb(lv_event_t* e)
{
	lv_obj_t * obj = lv_event_get_target(e);
	uint8_t* data = (uint8_t*)lv_event_get_user_data(e);
	bool state = lv_obj_has_state(obj, LV_STATE_CHECKED);
	
	if (data == &systemconfig.bluetooth.server_enabled)
	{
		if (state)
		{
			ble_enable();
		}
		else ble_disable();
		
		if (systemconfig.bluetooth.server_enabled) lv_obj_add_state(ui_settings.ui_bluetooth.status, LV_STATE_CHECKED);
		else lv_obj_clear_state(ui_settings.ui_bluetooth.status, LV_STATE_CHECKED);
	}
	else if (data == &systemconfig.opc.status)
	{
		
	}
	else
	{
		*data = state;
	}
}

void ui_settings_event_load_cb(lv_event_t* e)
{
	if (!load_configuration())
	{
		ui_show_messagebox(MESSAGEBOX_ERROR, "Can't load configuration from sd card.", 3000);
	}
	else
	{
		ui_show_messagebox(MESSAGEBOX_INFO, "Successful load configuration", 3000);
		ui_settings_update_configuratiion();
	}
}
void ui_settings_reboot_timer_cb(lv_timer_t * timer)
{
	esp_restart();
}

void ui_settings_event_save_cb(lv_event_t* e)
{
	IsInitialized = 0;
	uint8_t rx1_pin, rx2_pin, tx1_pin, tx2_pin;
	rx1_pin = get_value_from_index(lv_dropdown_get_selected(ui_settings.ui_serial1.ui_rx_pin));
	tx1_pin = get_value_from_index(lv_dropdown_get_selected(ui_settings.ui_serial1.ui_tx_pin));
	rx2_pin = get_value_from_index(lv_dropdown_get_selected(ui_settings.ui_serial2.ui_rx_pin));
	tx2_pin = get_value_from_index(lv_dropdown_get_selected(ui_settings.ui_serial2.ui_tx_pin));
	
	if (rx1_pin == rx2_pin || tx1_pin == tx2_pin)
	{
		ui_show_messagebox(MESSAGEBOX_ERROR, "Wrong serial configuration", 1000);
		return;
	}
	systemconfig.serial1.rx_pin = rx1_pin;
	systemconfig.serial1.tx_pin = tx1_pin;
	systemconfig.serial2.rx_pin = rx2_pin;
	systemconfig.serial2.tx_pin = tx2_pin;
	
	systemconfig.serial1.baud = get_baud_from_index(lv_dropdown_get_selected(ui_settings.ui_serial1.ui_baud));
	systemconfig.serial2.baud = get_baud_from_index(lv_dropdown_get_selected(ui_settings.ui_serial2.ui_baud));
	systemconfig.serial2.mode = lv_dropdown_get_selected(ui_settings.ui_serial2.ui_mode);
	
	
	if (!save_configuration())
	{
		ui_show_messagebox(MESSAGEBOX_ERROR, "Can't save configuration.", 3000);
		IsInitialized = 1;
	}
	else
	{
		ui_show_messagebox(MESSAGEBOX_INFO,
			"Settings Saved Succesfully, Rebooting NOW.", 5000);
		
		//serial_uart_update_config(UART_NUM_1, systemconfig.serial.tx_pin, systemconfig.serial.rx_pin, systemconfig.serial.baud);
		lv_timer_create(ui_settings_reboot_timer_cb, 5000, NULL);
	}
}


void ui_settigs_event_serial_button_cb(lv_event_t* e)
{
	int code = (int)e->user_data;
	switch (code)
	{
	case 0:
		if(run_mode != RUN_BLE_CLIENT)
			communication_tx_commandline(MasterCommPort, KEYBOARD_SHFIT_ADDRESS_STRING); 
		break;
	case 1:
		if (run_mode != RUN_BLE_CLIENT)
			communication_tx_commandline(MasterCommPort, KEYBOARD_RESET_ADDRESS_STRING); 
		break;
	}
}
void ui_settings_bluetooth_page_init()
{
	ui_settings_bluetooth_page = lv_obj_create(ui_settings_screen);
	lv_obj_set_size(ui_settings_bluetooth_page, 375, 225);//480-105
	lv_obj_set_pos(ui_settings_bluetooth_page, 102, 55); 
	lv_obj_set_style_pad_all(ui_settings_bluetooth_page, 10, LV_PART_MAIN);
	lv_obj_t* obj = ui_create_label(ui_settings_bluetooth_page, "Bluetooth", &lv_font_montserrat_20);
	lv_obj_set_align(obj, LV_ALIGN_TOP_MID);
	
	uint16_t y = 40;
	obj = ui_create_label(ui_settings_bluetooth_page, "Status: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y+10);
	obj = lv_switch_create(ui_settings_bluetooth_page);
	lv_obj_set_pos(obj, 160, y);
	if (systemconfig.bluetooth.server_enabled) lv_obj_add_state(obj, LV_STATE_CHECKED);
	else lv_obj_add_state(obj, LV_STATE_CHECKED);
	lv_obj_add_event_cb(obj, ui_settings_event_switch_cb, LV_EVENT_VALUE_CHANGED, &systemconfig.bluetooth.server_enabled);
	ui_settings.ui_bluetooth.status = obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_bluetooth_page, "Autostart: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y+10);
	obj = lv_switch_create(ui_settings_bluetooth_page);
	lv_obj_set_pos(obj, 160, y);
	if (systemconfig.bluetooth.autostart) lv_obj_add_state(obj, LV_STATE_CHECKED);
	else lv_obj_add_state(obj, LV_STATE_CHECKED);
	lv_obj_add_event_cb(obj, ui_settings_event_switch_cb, LV_EVENT_VALUE_CHANGED, &systemconfig.bluetooth.autostart);
	ui_settings.ui_bluetooth.autostart = obj;

	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_bluetooth_page, "Screen Control: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_switch_create(ui_settings_bluetooth_page);
	lv_obj_set_pos(obj, 160, y);
	if (systemconfig.ScreenControlEnabled) lv_obj_add_state(obj, LV_STATE_CHECKED);
	else lv_obj_clear_state(obj, LV_STATE_CHECKED);
	lv_obj_add_event_cb(obj, ui_settings_event_switch_cb, LV_EVENT_VALUE_CHANGED, &systemconfig.ScreenControlEnabled);
	ui_settings.ui_bluetooth.screen_control = obj;
}

void ui_settings_wifi_page_init()
{
	ui_settings_wifi_page = lv_obj_create(ui_settings_screen);
	lv_obj_set_size(ui_settings_wifi_page, 375, 225); //480-105
	lv_obj_set_pos(ui_settings_wifi_page, 102, 55); 
	lv_obj_set_style_pad_all(ui_settings_wifi_page, 10, LV_PART_MAIN);
	lv_obj_t* obj = ui_create_label(ui_settings_wifi_page, "WIFI", &lv_font_montserrat_20);
	lv_obj_set_align(obj, LV_ALIGN_TOP_MID);
	
	uint16_t y = 40;
	obj = ui_create_label(ui_settings_wifi_page, "WIFI SSID: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	
	obj = lv_textarea_create(ui_settings_wifi_page);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_BACKGROUND_COLOR), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
	lv_textarea_set_one_line(obj, true);
	lv_obj_set_width(obj, 150);
	lv_obj_set_pos(obj, 160, y);
	lv_obj_add_event_cb(obj, ui_event_edit_cb, LV_EVENT_ALL, NULL);
	lv_textarea_set_text(obj, (const char*)systemconfig.wifi.ssid);
	lv_obj_set_user_data(obj, systemconfig.wifi.ssid);
	ui_settings.ui_wifi.ssid = obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_wifi_page, "WIFI PASSWORD: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_textarea_create(ui_settings_wifi_page);	
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_BACKGROUND_COLOR), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
	lv_textarea_set_one_line(obj, true);
	lv_textarea_set_password_mode(obj, true);
	lv_obj_set_width(obj, 150);
	lv_obj_set_pos(obj, 160, y);
	lv_obj_add_event_cb(obj, ui_event_edit_cb, LV_EVENT_ALL,NULL);
	lv_textarea_set_text(obj, (const char*)systemconfig.wifi.password);
	lv_obj_set_user_data(obj, systemconfig.wifi.password);
	ui_settings.ui_wifi.password = obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_wifi_page, "Auto connect: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 5);
	obj = lv_switch_create(ui_settings_wifi_page);
	lv_obj_set_pos(obj, 160, y);
	if (systemconfig.wifi.autoconnect) lv_obj_add_state(obj, LV_STATE_CHECKED);
	else lv_obj_add_state(obj, LV_STATE_CHECKED);
	lv_obj_add_event_cb(obj, ui_settings_event_switch_cb, LV_EVENT_VALUE_CHANGED, &systemconfig.wifi.autoconnect);
	ui_settings.ui_wifi.autoconnect = obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_wifi_page, "Status: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_switch_create(ui_settings_wifi_page);	
	lv_obj_set_pos(obj, 160, y);
	if (systemconfig.wifi.autoconnect) lv_obj_add_state(obj, LV_STATE_CHECKED);
	else lv_obj_add_state(obj, LV_STATE_CHECKED);	
	lv_obj_add_event_cb(obj, ui_settings_event_switch_cb, LV_EVENT_VALUE_CHANGED, &systemconfig.wifi.status);
	ui_settings.ui_wifi.status = obj;
	
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_wifi_page, "IP: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 5);
	obj = lv_label_create(ui_settings_wifi_page);
	lv_obj_set_pos(obj, 160, y);
	lv_label_set_text(obj, (const char*)systemconfig.wifi.ip);
	
	ui_settings.ui_wifi.ip= obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_wifi_page, "SUBNET: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 5);
	obj = lv_label_create(ui_settings_wifi_page);
	lv_obj_set_pos(obj, 160, y);
	lv_label_set_text(obj, (const char*)systemconfig.wifi.subnet);
	lv_obj_add_event_cb(obj, ui_settings_event_switch_cb, LV_EVENT_VALUE_CHANGED, &systemconfig.wifi.autoconnect);
	ui_settings.ui_wifi.subnet= obj;
}

void ui_settings_opc_page_init()
{
	ui_settings_opc_page = lv_obj_create(ui_settings_screen);
	lv_obj_set_size(ui_settings_opc_page, 375, 225); //480-105
	lv_obj_set_pos(ui_settings_opc_page, 102, 55); 
	lv_obj_set_style_pad_all(ui_settings_opc_page, 10, LV_PART_MAIN);
	lv_obj_t* obj = ui_create_label(ui_settings_opc_page, "OPC", &lv_font_montserrat_20);
	lv_obj_set_align(obj, LV_ALIGN_TOP_MID);
	
	uint16_t y = 40;
	obj = ui_create_label(ui_settings_opc_page, "User name: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_textarea_create(ui_settings_opc_page);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_BACKGROUND_COLOR), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
	lv_textarea_set_one_line(obj, true);
	lv_obj_set_width(obj, 150);
	lv_obj_set_pos(obj, 160, y);
	lv_obj_add_event_cb(obj, ui_event_edit_cb, LV_EVENT_ALL, NULL);
	lv_obj_set_user_data(obj, systemconfig.opc.username);
	lv_textarea_set_text(obj, (const char*)systemconfig.opc.username);
	ui_settings.ui_opc.name = obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_opc_page, "User password: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_textarea_create(ui_settings_opc_page);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_BACKGROUND_COLOR), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
	lv_textarea_set_one_line(obj, true);
	lv_textarea_set_password_mode(obj, true);
	lv_obj_set_width(obj, 150);
	lv_obj_set_pos(obj, 160, y);
	lv_obj_add_event_cb(obj, ui_event_edit_cb, LV_EVENT_ALL, NULL);
	lv_obj_set_user_data(obj, systemconfig.opc.userpassword);
	lv_textarea_set_text(obj, (const char*)systemconfig.opc.userpassword);
	ui_settings.ui_opc.password = obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_opc_page, "Status: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_switch_create(ui_settings_opc_page);
	lv_obj_set_pos(obj, 160, y);
	if (systemconfig.opc.status) lv_obj_add_state(obj, LV_STATE_CHECKED);
	else lv_obj_add_state(obj, LV_STATE_CHECKED);
	lv_obj_add_event_cb(obj, ui_settings_event_switch_cb, LV_EVENT_VALUE_CHANGED, &systemconfig.opc.status);
	ui_settings.ui_opc.status = obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_opc_page, "Autostart: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_switch_create(ui_settings_opc_page);
	lv_obj_set_pos(obj, 160, y);
	if (systemconfig.opc.autostart) lv_obj_add_state(obj, LV_STATE_CHECKED);
	else lv_obj_add_state(obj, LV_STATE_CHECKED);
	lv_obj_add_event_cb(obj, ui_settings_event_switch_cb, LV_EVENT_VALUE_CHANGED, &systemconfig.opc.autostart);
	ui_settings.ui_opc.autostart = obj;
}


void ui_settings_serial_page_init(int index)
{
	UI_SERIAL* ui_serial = index == 0 ? &ui_settings.ui_serial1 : &ui_settings.ui_serial2;
	SERIAL_CONFIG* serialConf = index == 0 ? &systemconfig.serial1 : &systemconfig.serial2;
	
	char szpins[100] = "GPIO_10\nGPIO_11\nGPIO_12\nGPIO_13\nGPIO_14\nGPIO_21";
	lv_obj_t* obj = lv_obj_create(ui_settings_screen);
	lv_obj_set_size(obj, 375, 225); //480-105
	lv_obj_set_pos(obj, 102, 55);
	lv_obj_set_style_pad_all(obj, 10, LV_PART_MAIN);
	if (index == 0) ui_settings_serial_page1 = obj;
	else ui_settings_serial_page2 = obj;
	lv_obj_t* page = obj;
	
	obj = ui_create_label(page, index == 0? "Serial 1" : "Serial 2", &lv_font_montserrat_20);
	lv_obj_set_align(obj, LV_ALIGN_TOP_MID);
	
	uint16_t x = 0, y = 30;	
	obj = ui_create_label(page, "RX_PIN: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y);
	obj = lv_dropdown_create(page);
	lv_dropdown_set_options(obj, szpins);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_TEXTAREA_BORDER_COLOR), LV_PART_MAIN);
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_width(obj, 100);
	lv_obj_set_pos(obj, 70, y);
	lv_dropdown_set_selected(obj, get_index_from_value(serialConf->rx_pin));
	ui_serial->ui_rx_pin = obj;
	
	y += 35;
	obj = ui_create_label(page, "TX_PIN: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, x, y);
	obj = lv_dropdown_create(page);
	lv_dropdown_set_options(obj, szpins);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_TEXTAREA_BORDER_COLOR), LV_PART_MAIN);
	lv_obj_set_style_pad_all(obj, 3, LV_PART_MAIN);
	lv_obj_set_width(obj, 100);
	lv_obj_set_pos(obj, 70, y);
	ui_serial->ui_tx_pin = obj;
	lv_dropdown_set_selected(obj, get_index_from_value(serialConf->tx_pin));
	
	y += 35;
	obj = ui_create_label(page, "BAUD: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y);
	obj = lv_dropdown_create(page);
	lv_dropdown_set_options(obj, "9600\n14400\n19200\n38400\n57600\n115200\n230400\n460800\n921600\n1000000");
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_TEXTAREA_BORDER_COLOR), LV_PART_MAIN);
	lv_obj_set_style_pad_all(obj, 3, LV_PART_MAIN);
	lv_obj_set_width(obj, 100);
	lv_obj_set_pos(obj, 70, y);
	
	lv_dropdown_set_selected(obj, get_index_from_baud(serialConf->baud));
	ui_serial->ui_baud = obj;

	if (index == 1)
	{
		y += 35;
		obj = ui_create_label(page, "Mode: ", &lv_font_montserrat_14);
		lv_obj_set_pos(obj, 0, y);
		obj = lv_dropdown_create(page);
		lv_dropdown_set_options(obj, "SIMPLE\nSECS\nBLE Modern Mode");
		lv_obj_set_style_border_color(obj, lv_color_hex(UI_TEXTAREA_BORDER_COLOR), LV_PART_MAIN);
		lv_obj_set_style_pad_all(obj, 3, LV_PART_MAIN);
		lv_obj_set_width(obj, 170);
		lv_obj_set_pos(obj, 70, y);
	
		lv_dropdown_set_selected(obj, serialConf->mode);
		ui_serial->ui_mode = obj;
	}
	
	y += 35;
	obj = ui_create_button(page, "Preset K", 80, 40, 3, &lv_font_montserrat_14, ui_settings_serial_load_event_cb, (void*)0);
	lv_obj_set_pos(obj, 2, y);
	obj = ui_create_button(page, "Preset L", 80, 40, 3, &lv_font_montserrat_14, ui_settings_serial_load_event_cb, (void*)1);
	lv_obj_set_pos(obj, 95, y);
	
	if (index == 0)
	{
		y = 35;
		sprintf(ui_temp_string, "CAN ADDRESS: %02d", systemconfig.can_address);
		obj = ui_create_label(page, ui_temp_string, &lv_font_montserrat_16);
		lv_obj_set_style_text_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN);
		lv_obj_set_pos(obj, 200, y);
		y += 30;
		obj = ui_create_button(page, "SHIFT ADDRESS", 150, 40, 3, &lv_font_montserrat_14, ui_settigs_event_serial_button_cb, (void*)0); //ui_settigs_event_serial_button_cb
		lv_obj_set_pos(obj, 200, y);
		y += 45;
		obj = ui_create_button(page, "RESET ADDRESS", 150, 40, 3, &lv_font_montserrat_14, ui_settigs_event_serial_button_cb, (void*)1); //ui_settigs_event_serial_button_cb
		lv_obj_set_pos(obj, 200, y);	
	}
}

void ui_settings_secs_page_init()
{
	ui_settings_secs_page = lv_obj_create(ui_settings_screen);
	lv_obj_set_size(ui_settings_secs_page, 375, 225); //480-105
	lv_obj_set_pos(ui_settings_secs_page, 102, 55); 
	lv_obj_set_style_pad_all(ui_settings_secs_page, 10, LV_PART_MAIN);
	lv_obj_t* obj = ui_create_label(ui_settings_secs_page, "SECS", &lv_font_montserrat_20);
	lv_obj_set_align(obj, LV_ALIGN_TOP_MID);
	
	uint16_t y = 40;
	obj = ui_create_label(ui_settings_secs_page, "Reload timer1(s): ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_textarea_create(ui_settings_secs_page);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_TEXTAREA_BORDER_COLOR), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
	lv_textarea_set_one_line(obj, true);
	lv_obj_set_width(obj, 50);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_RIGHT);
	lv_obj_set_pos(obj, 160, y);
	lv_obj_add_event_cb(obj, ui_event_edit_cb, LV_EVENT_ALL, (void*)1);
	ui_textarea_set_nmuber(obj, systemconfig.secs.timerReload1);
	lv_obj_set_user_data(obj, (void*)&systemconfig.secs.timerReload1);
	lv_textarea_set_placeholder_text(obj, "Number");
	ui_settings.ui_secs.ui_timer_reload1 = obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_secs_page, "Reload timer2(s):", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_textarea_create(ui_settings_secs_page);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_TEXTAREA_BORDER_COLOR), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_RIGHT);
	lv_textarea_set_one_line(obj, true);
	lv_obj_set_width(obj, 50);
	lv_obj_set_pos(obj, 160, y);
	lv_obj_add_event_cb(obj, ui_event_edit_cb, LV_EVENT_ALL, (void*)1);
	ui_textarea_set_nmuber(obj, systemconfig.secs.timerReload2);
	lv_obj_set_user_data(obj, (void*)&systemconfig.secs.timerReload2);
	ui_settings.ui_secs.ui_timer_reload2 = obj;
	
	y += SETTINGS_LINE_SPACE;
	obj = ui_create_label(ui_settings_secs_page, "Retry timer(s):  ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y + 10);
	obj = lv_textarea_create(ui_settings_secs_page);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_TEXTAREA_BORDER_COLOR), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
	lv_textarea_set_one_line(obj, true);
	lv_obj_set_width(obj, 50);	
	lv_textarea_set_align(obj, LV_TEXT_ALIGN_RIGHT);
	lv_obj_set_pos(obj, 160, y);
	lv_obj_add_event_cb(obj, ui_event_edit_cb, LV_EVENT_ALL, (void*)1);
	lv_obj_set_user_data(obj, (void*)&systemconfig.secs.timerRetry);
	ui_textarea_set_nmuber(obj, systemconfig.secs.timerRetry);
	ui_settings.ui_secs.ui_timer_retry = obj;
}

void ui_settings_system_page_init()
{
	ui_settings_system_page = lv_obj_create(ui_settings_screen);
	lv_obj_set_size(ui_settings_system_page, 375, 225); //480-105
	lv_obj_set_pos(ui_settings_system_page, 102, 55); 
	lv_obj_set_style_pad_all(ui_settings_system_page, 10, LV_PART_MAIN);
	lv_obj_t* obj = ui_create_label(ui_settings_system_page, "SYSTEM", &lv_font_montserrat_20);
	lv_obj_set_align(obj, LV_ALIGN_TOP_MID);
	
	uint16_t y = 40, step = SETTINGS_LINE_SPACE - 10;

	obj = ui_create_label(ui_settings_system_page, "Revision: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y);
	obj = ui_create_label(ui_settings_system_page, MajorStep, &lv_font_montserrat_14);	
	lv_obj_set_pos(obj, 160, y);
	
	y += step;
	obj = ui_create_label(ui_settings_system_page, "Release date: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y);
	obj = ui_create_label(ui_settings_system_page, RevisionDate, &lv_font_montserrat_14);	
	lv_obj_set_pos(obj, 160, y);
	
	y += step;
	obj = ui_create_label(ui_settings_system_page, "CPU: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y);
	
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	
	// CPU Speed - 80Mhz / 160 Mhz / 240Mhz
	rtc_cpu_freq_config_t conf;
	rtc_clk_cpu_freq_get_config(&conf);

	
	char info[1024] = { 0 };
	sprintf(info,
		"%s Rev. %d\n%s %dMHz",
		CONFIG_IDF_TARGET,
		(int)chip_info.revision, 
		chip_info.cores == 2 ? "Dual Core" : "Single Core",
		(int)conf.freq_mhz);
	obj = ui_create_label(ui_settings_system_page, info, &lv_font_montserrat_14);	
	lv_obj_set_pos(obj, 160, y);
	
	
	y += step + 10;
	obj = ui_create_label(ui_settings_system_page, "Flash: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y);
	uint32_t flash_size;
	if (esp_flash_get_size(NULL, &flash_size) == ESP_OK)
	{
		sprintf(info,
			"%dMB %s", 
			(int)flash_size / (1024 * 1024),
			(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "[embedded]" : "[external]");
	}
	else
	{
		strcpy(info, "");
	}
	obj = ui_create_label(ui_settings_system_page, info, &lv_font_montserrat_14);	
	lv_obj_set_pos(obj, 160, y);
	
	y += step;
	obj = ui_create_label(ui_settings_system_page, "Wifi: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y);
	obj = ui_create_label(ui_settings_system_page, (char*)((chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "2.4GHz WIFI" : "NA"), &lv_font_montserrat_14);	
	lv_obj_set_pos(obj, 160, y);
	
	y += step;
	obj = ui_create_label(ui_settings_system_page, "Bluetooth: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 0, y);
	sprintf(info, "%s%s", (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "", (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
	obj = ui_create_label(ui_settings_system_page, info, &lv_font_montserrat_14);	
	lv_obj_set_pos(obj, 160, y);
}


void ui_settings_screen_init()
{
	ui_settings_screen = ui_create_screen();	
	ui_create_pct_title(ui_settings_screen,
		SCREEN_SETTINGS);// ui_create_titlebar(ui_settings_screen, TITLEBAR_BACKGROUND_COLOR);
	
	lv_obj_t* title_label = lv_label_create(ui_settings_screen);	
	lv_obj_set_width(title_label, LV_SIZE_CONTENT);
	lv_obj_set_height(title_label, LV_SIZE_CONTENT);
	lv_label_set_recolor(title_label, true);
	lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_label_set_text(title_label, "SETTINGS");
	lv_obj_set_style_text_font(title_label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);	
	lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 5);
	
	lv_obj_t* submenu = lv_obj_create(ui_settings_screen);
	//lv_obj_clear_flag(submenu, LV_OBJ_FLAG_SCROLLABLE); /// Flags
	lv_obj_set_size(submenu, 100, 320);
	lv_obj_set_pos(submenu, 0, 0);
	lv_obj_set_style_border_width(submenu, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(submenu, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(submenu, 2, LV_PART_MAIN);
	lv_obj_set_style_bg_color(submenu, lv_color_hex(0), LV_PART_MAIN);
	
	int y = 2, step = 45, h = 40;
	lv_obj_t* obj = ui_create_button(submenu, "Bluetooth", LV_PCT(90), h, 3, &lv_font_montserrat_14, ui_settings_event_submenu_cb, (void*)SETTINGS_SUBMENU_BLUETOOTH);
	lv_obj_set_user_data(obj, (void*)SETTINGS_SUBMENU_BLUETOOTH);
	lv_obj_set_style_bg_color(obj, lv_color_hex(UI_BUTTON_ACTIVE_BG_COLOR), LV_PART_MAIN);
	lv_obj_set_pos(obj, 0, y);
	settings_active_menu = obj;
	obj = ui_create_button(submenu, "WIFI", LV_PCT(90), h, 3, &lv_font_montserrat_14, ui_settings_event_submenu_cb, (void*)SETTINGS_SUBMENU_WIFI);
	lv_obj_set_user_data(obj, (void*)SETTINGS_SUBMENU_BLUETOOTH);
	y += step; lv_obj_set_pos(obj, 0, y);
	obj = ui_create_button(submenu, "OPC", LV_PCT(90), h, 3, &lv_font_montserrat_14, ui_settings_event_submenu_cb, (void*)SETTINGS_SUBMENU_OPC);
	lv_obj_set_user_data(obj, (void*)SETTINGS_SUBMENU_OPC);
	y += step; lv_obj_set_pos(obj, 0, y);
	obj = ui_create_button(submenu, "SERIAL1", LV_PCT(90), h, 3, &lv_font_montserrat_14, ui_settings_event_submenu_cb, (void*)SETTINGS_SUBMENU_SERIAL1);
	lv_obj_set_user_data(obj, (void*)SETTINGS_SUBMENU_SERIAL1);
	y += step; lv_obj_set_pos(obj, 0, y);
	
	obj = ui_create_button(submenu, "SERIAL2", LV_PCT(90), h, 3, &lv_font_montserrat_14, ui_settings_event_submenu_cb, (void*)SETTINGS_SUBMENU_SERIAL2);
	lv_obj_set_user_data(obj, (void*)SETTINGS_SUBMENU_SERIAL1);
	y += step; lv_obj_set_pos(obj, 0, y);
	
	obj = ui_create_button(submenu, "SECS", LV_PCT(90), h, 3, &lv_font_montserrat_14, ui_settings_event_submenu_cb, (void*)SETTINGS_SUBMENU_SECS);
	lv_obj_set_user_data(obj, (void*)SETTINGS_SUBMENU_SECS);
	y += step; lv_obj_set_pos(obj, 0, y);
	
	obj = ui_create_button(submenu, "SYSTEM", LV_PCT(90), h, 3, &lv_font_montserrat_14, ui_settings_event_submenu_cb, (void*)SETTINGS_SUBMENU_SYSTEM);
	lv_obj_set_user_data(obj, (void*)SETTINGS_SUBMENU_SYSTEM);
	y += step; lv_obj_set_pos(obj, 0, y);

	ui_settings_bluetooth_page_init();
	settings_active_page = ui_settings_bluetooth_page;
	ui_settings_wifi_page_init();
	lv_obj_add_flag(ui_settings_wifi_page, LV_OBJ_FLAG_HIDDEN);
	ui_settings_opc_page_init();
	lv_obj_add_flag(ui_settings_opc_page, LV_OBJ_FLAG_HIDDEN);
	ui_settings_serial_page_init(0);
	lv_obj_add_flag(ui_settings_serial_page1, LV_OBJ_FLAG_HIDDEN);
	ui_settings_serial_page_init(1);
	lv_obj_add_flag(ui_settings_serial_page2, LV_OBJ_FLAG_HIDDEN);
	ui_settings_secs_page_init();
	lv_obj_add_flag(ui_settings_secs_page, LV_OBJ_FLAG_HIDDEN);
	
	ui_settings_system_page_init();
	lv_obj_add_flag(ui_settings_system_page, LV_OBJ_FLAG_HIDDEN);
	
	obj = ui_create_button(ui_settings_screen, "Save & Reboot", 133, 35, 3, &lv_font_montserrat_14, ui_settings_event_save_cb, NULL);
	lv_obj_set_pos(obj, 102, 285);
	obj = ui_create_button(ui_settings_screen, "Load", 239, 35, 3, &lv_font_montserrat_14, ui_settings_event_load_cb, NULL);
	lv_obj_set_pos(obj, 238, 285);
	
	ui_settings_update_configuratiion();
	ui_settings_initialized = true;
}

void ui_settings_update_configuratiion()
{
	if (systemconfig.bluetooth.autostart)	lv_obj_add_state(ui_settings.ui_bluetooth.autostart, LV_STATE_CHECKED);
	else lv_obj_clear_state(ui_settings.ui_bluetooth.autostart, LV_STATE_CHECKED);
	if (systemconfig.bluetooth.server_enabled)	lv_obj_add_state(ui_settings.ui_bluetooth.status, LV_STATE_CHECKED);
	else lv_obj_clear_state(ui_settings.ui_bluetooth.status, LV_STATE_CHECKED);
	
	if (systemconfig.wifi.autoconnect)	lv_obj_add_state(ui_settings.ui_wifi.autoconnect, LV_STATE_CHECKED);
	else lv_obj_clear_state(ui_settings.ui_wifi.autoconnect, LV_STATE_CHECKED);
	if (systemconfig.wifi.status)	lv_obj_add_state(ui_settings.ui_wifi.status, LV_STATE_CHECKED);
	else lv_obj_clear_state(ui_settings.ui_wifi.status, LV_STATE_CHECKED);
	lv_textarea_set_text(ui_settings.ui_wifi.ssid, (const char*)systemconfig.wifi.ssid);
	lv_textarea_set_text(ui_settings.ui_wifi.password, (const char*)systemconfig.wifi.password);	
	lv_label_set_text(ui_settings.ui_wifi.ip, (const char*)systemconfig.wifi.ip);
	lv_label_set_text(ui_settings.ui_wifi.subnet, (const char*)systemconfig.wifi.subnet);

	
	if (systemconfig.opc.autostart)	lv_obj_add_state(ui_settings.ui_opc.autostart, LV_STATE_CHECKED);
	else lv_obj_clear_state(ui_settings.ui_opc.autostart, LV_STATE_CHECKED);
	if (systemconfig.opc.status)	lv_obj_add_state(ui_settings.ui_opc.status, LV_STATE_CHECKED);
	else lv_obj_clear_state(ui_settings.ui_opc.status, LV_STATE_CHECKED);
	lv_textarea_set_text(ui_settings.ui_opc.name, (const char*)systemconfig.opc.username);
	lv_textarea_set_text(ui_settings.ui_opc.password, (const char*)systemconfig.opc.userpassword);
	
}
