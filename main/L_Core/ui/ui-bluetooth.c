#include "main.h"
#include "K_Core/K_Core.h"
#include "L_Core/bluetooth/ble.h"
#include "L_Core/storage/nvs.h"
#include "ui-bluetooth.h"

lv_obj_t* ui_ble_screen;
lv_obj_t* ui_ble_btn_scan;
lv_obj_t* ui_ble_spinner_scan;
lv_obj_t* ui_ble_device_list;
lv_obj_t* ui_ble_device_detail;
lv_obj_t* ui_ble_pair_button;
lv_obj_t* ui_ble_device_name;
lv_obj_t* ui_ble_device_address;
lv_obj_t* ui_ble_send;
lv_obj_t* ui_ble_receive;
lv_obj_t* ui_ble_total_received;
lv_obj_t* ui_ble_server_button;
lv_obj_t* ui_ble_client_button;
lv_obj_t* ui_ble_server_panel;
lv_obj_t* ui_ble_client_panel;
lv_obj_t* ui_ble_server_send_text;
lv_obj_t* ui_ble_server_sent_total;
lv_obj_t* ui_ble_server_receive_total;
lv_obj_t* ui_ble_server_sent_status;
lv_obj_t* ui_ble_server_receive_status;
lv_obj_t* ui_ble_server_name;

lv_obj_t* ui_ble_msg;
lv_obj_t* ui_ble_address_spin;
lv_obj_t* ui_ble_address_name;

BleRemoteDevice* selected_device = NULL;
ble_server_status_t  prev_ble_server_status = BLE_SERVER_LISTENING;
// screen: 0: server screen
//			1: client screen
void ui_ble_switch_screen(uint8_t screen)
{
	if (screen == 0)
	{
		lv_obj_clear_flag(ui_ble_server_panel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(ui_ble_client_panel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_set_style_bg_color(ui_ble_server_button, lv_color_hex(UI_MENU_ACTIVE_ITEM_COLOR), LV_PART_MAIN);
		lv_obj_set_style_bg_color(ui_ble_client_button, lv_color_hex(UI_CHECK_NONACTIVE_COLOR), LV_PART_MAIN);
	}
	else
	{
		lv_obj_add_flag(ui_ble_server_panel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(ui_ble_client_panel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_set_style_bg_color(ui_ble_server_button, lv_color_hex(UI_CHECK_NONACTIVE_COLOR), LV_PART_MAIN);
		lv_obj_set_style_bg_color(ui_ble_client_button, lv_color_hex(UI_MENU_ACTIVE_ITEM_COLOR), LV_PART_MAIN);
	}
}

void ui_ble_server_send_event_cb(lv_event_t* e) 
{
	char* text = (char*)lv_textarea_get_text(ui_ble_server_send_text);
	uint8_t len = strlen(text);
	if (len == 0) return;
	sprintf(ui_temp_string, "%s\n", text);
	ble_server_send_data((uint8_t*)ui_temp_string, len+1);
}

void ui_ble_show_setting_address()
{
	lv_obj_clear_flag(ui_ble_msg, LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text(ui_ble_address_name, ble_get_name());
	lv_spinbox_set_value(ui_ble_address_spin, systemconfig.server_base_address);
}

void ui_ble_setting_button_event_cb(lv_event_t* e)
{
	int code = (int)e->user_data;
	if (!code)
	{
		systemconfig.server_base_address = lv_spinbox_get_value(ui_ble_address_spin);
		save_configuration();
		ble_update_base_address();
		lv_label_set_text(ui_ble_server_name, lv_label_get_text(ui_ble_address_name));
	}
	lv_obj_add_flag(ui_ble_msg, LV_OBJ_FLAG_HIDDEN);
}

void ui_ble_spinbox_event_cb(lv_event_t* e)
{
	int code = (int)e->user_data;
	if (code)
	{
		lv_spinbox_increment(ui_ble_address_spin);
	}
	else 
		lv_spinbox_decrement(ui_ble_address_spin);
	
	strcpy(ui_temp_string, ble_get_name());
	int val = lv_spinbox_get_value(ui_ble_address_spin);
	char sz[4] = { 0 };
	
	sprintf(sz, "%03d",  val);
	strncpy(ui_temp_string + 9, sz, 3);
	lv_label_set_text(ui_ble_address_name, ui_temp_string);
}
void ui_ble_create_window_set_address()
{
	lv_obj_t *msgbox = ui_create_panel(ui_ble_screen, 0x444444, false);
	lv_obj_set_size(msgbox, 290, 210); // Resize the message box
	lv_obj_center(msgbox);
	
	uint16_t x = 10, y = 10, sh = 40, gap = 5;
	lv_obj_t* obj = ui_create_label(msgbox, "Please choose the number of the server base address.", &lv_font_montserrat_16);
	lv_obj_set_width(obj, 250);
	lv_obj_set_pos(obj, x+10, y); // Resize the message box
	
	y += sh*1.2 + gap;
	
	obj = ui_create_label(msgbox, ble_get_name(), &lv_font_montserrat_20);
	ui_ble_address_name = obj;
	lv_obj_set_pos(obj, 40, y);
	
	y += 40;
	x += 40;
	obj = lv_btn_create(msgbox);
	lv_obj_set_size(obj, sh, sh);
	lv_obj_set_style_bg_img_src(obj, LV_SYMBOL_MINUS, 0);
	lv_obj_add_event_cb(obj, ui_ble_spinbox_event_cb, LV_EVENT_CLICKED, (void*)0);

	lv_obj_set_pos(obj, x, y);
	
	x += sh + gap;
	obj = lv_spinbox_create(msgbox);
	lv_spinbox_set_range(obj, 0, 255);
	lv_spinbox_set_digit_format(obj, 3, 0);
	lv_obj_set_style_pad_all(obj, 5, LV_PART_MAIN);
	lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN);
	lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_size(obj, 100, 40);
	lv_obj_set_pos(obj, x, y );
	ui_ble_address_spin = obj;
	x += 100 + gap;
	obj = lv_btn_create(msgbox);
	lv_obj_set_size(obj, sh, sh);
	lv_obj_set_style_bg_img_src(obj, LV_SYMBOL_PLUS, 0);
	lv_obj_set_pos(obj, x, y);
	lv_obj_add_event_cb(obj, ui_ble_spinbox_event_cb, LV_EVENT_CLICKED, (void*)1);
	x = 100; y += sh + gap* 3;
	obj = ui_create_button(msgbox, "APPLY", 80, 40, 3, &lv_font_montserrat_14, ui_ble_setting_button_event_cb, (void*)0);
	lv_obj_set_pos(obj, x, y);
	x += 80+gap;
	obj = ui_create_button(msgbox, "CANCEL", 80, 40, 3, &lv_font_montserrat_14, ui_ble_setting_button_event_cb, (void*)1);
	lv_obj_set_pos(obj, x, y);
	ui_ble_msg = msgbox;
	
	
	
	lv_obj_add_flag(ui_ble_msg, LV_OBJ_FLAG_HIDDEN);
}

void ui_ble_server_address_event_cb(lv_event_t* e)
{
	
	ui_ble_show_setting_address();
}
void ui_ble_disconnect_event_cb(lv_event_t* e)
{
	ble_server_disconnect();
}

void ui_ble_server_updatename_event_cb(lv_event_t* e)
{
	requestBleNameTo407();
}
void ui_ble_scan_event_cb(lv_event_t* e) 
{
	//lv_obj_t * label = lv_obj_get_child(target, 0);
	switch (ble_scan_status)
	{
	case BLE_CLIENT_SCAN_READY: ble_scan_start();break;
	case BLE_CLIENT_SCANNING: ble_scan_stop();break;
	default: break;
	}
}

void ui_ble_timer_handler(lv_timer_t* timer)
{
	if (ble_server_send_blink_count > 0)
	{
		sprintf(ui_temp_string, "%d", (int)ble_server_total_sent);
		lv_label_set_text(ui_ble_server_sent_total, ui_temp_string);
		lv_obj_set_style_text_color(ui_ble_server_sent_status, lv_color_hex(ble_server_send_blink_count % 2 ? UI_BUTTON_NORMAL_BG_COLOR : UI_BUTTON_NORMAL_FG_COLOR), LV_PART_MAIN);
		ble_server_send_blink_count--;
	}
	else {
		lv_obj_set_style_text_color(ui_ble_server_sent_status, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN);
	}
	if (ble_server_receive_blink_count > 0)
	{
		sprintf(ui_temp_string, "%d", (int)ble_server_total_received);
		lv_label_set_text(ui_ble_server_receive_total, ui_temp_string);
		lv_obj_set_style_text_color(ui_ble_server_receive_status, lv_color_hex(ble_server_receive_blink_count % 2 ? UI_BUTTON_NORMAL_BG_COLOR : UI_BUTTON_NORMAL_FG_COLOR), LV_PART_MAIN);
		ble_server_receive_blink_count--;
	}
	else
	{
		lv_obj_set_style_text_color(ui_ble_server_receive_status, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN);
	}
}
void ui_ble_event_device_item_cb(lv_event_t* e) 
{
	//lv_obj_t * target = lv_event_get_target(e);
	BleRemoteDevice* device = (BleRemoteDevice*)lv_event_get_user_data(e);
	lv_label_set_text(ui_ble_device_name, device->device_name);
	lv_label_set_text(ui_ble_device_address, device->address);
	selected_device = device;
	ui_ble_set_device_status(selected_device);
}

void ui_ble_send_event_cb(lv_event_t* e)
{
	lv_obj_t* textobj = (lv_obj_t*)lv_event_get_user_data(e);
	char* text = (char*)lv_textarea_get_text(textobj);
	sprintf(ui_temp_string, "%s\n", text);
	communication_add_string_to_ble_buffer(&bleDevice.TxBuffer, ui_temp_string);
}
void ui_ble_switch_event_cb(lv_event_t* e)
{
	uint8_t screen = (uint8_t)(int) e->user_data;	
	ui_ble_switch_screen(screen);
}

void ui_ble_pair_event_cb(lv_event_t* e)
{
	if(!selected_device) return;
	if(!selected_device->is_connected)
		ble_client_connect_device(selected_device);
	else
		ble_client_disconnect_device(selected_device);
}

void ui_ble_event_swap_cb(lv_event_t* e)
{
	ui_transform_screen(SCREEN_MEG);
}


void ui_ble_set_device_status(BleRemoteDevice* dev)
{
	lv_obj_t* item = ui_ble_get_item_by_device(dev);
	lv_obj_t* label = lv_obj_get_child(item, 0);
	if(dev->is_connected) {
		lv_label_set_text_fmt(label, "#66d800 " LV_SYMBOL_BLUETOOTH " %s #", dev->device_name);
		if(selected_device == dev) {
			lv_obj_set_style_bg_color(ui_ble_pair_button, lv_color_hex(0x3e8300), LV_PART_MAIN);
			label = lv_obj_get_child(ui_ble_pair_button, 0);
			lv_label_set_text(label, "UNPAIR DEVICE");
		}
	}else {
		lv_label_set_text_fmt(label, "#c4c4c4 " LV_SYMBOL_BLUETOOTH " %s #", dev->device_name);
		if(selected_device == dev) {
			lv_obj_set_style_bg_color(ui_ble_pair_button, lv_color_hex(0x4b4b4b), LV_PART_MAIN);
			label = lv_obj_get_child(ui_ble_pair_button, 0);
			lv_label_set_text(label, "PAIR DEVICE");
		}
	}
}

void ui_ble_screen_init()
{
	lv_obj_t* obj;
	ui_ble_screen = ui_create_screen();	
	ui_create_pct_title(ui_ble_screen, SCREEN_BLUETOOTH);
	
	obj = ui_create_label(ui_ble_screen, (char*)"BLUETOOTH", &mono_bold_28);	
	lv_obj_set_pos(obj, 250, 7);
	
	int x = 20, y = 70;
	int button_large_width = 100;
	int button_h = 45;
	int gap = 5;
	
	y = 60;
	obj = ui_create_button(ui_ble_screen, "SERVER", SCREEN_WIDTH / 2 - 1, button_h, 3, &lv_font_montserrat_14, ui_ble_switch_event_cb, (void*)0);	
	lv_obj_set_pos(obj, 0, y);
	ui_ble_server_button = obj;
	ui_change_button_color(obj, UI_MENU_ACTIVE_ITEM_COLOR, UI_BUTTON_NORMAL_FG_COLOR);
	obj = ui_create_button(ui_ble_screen, "CLIENT", SCREEN_WIDTH / 2 - 1, button_h, 3, &lv_font_montserrat_14, ui_ble_switch_event_cb, (void*)1);	
	lv_obj_set_pos(obj, SCREEN_WIDTH/2, y);
	ui_ble_client_button = obj;
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_NORMAL_FG_COLOR);
	// panel
	x = 5; y +=button_h + gap;
	obj = lv_obj_create(ui_ble_screen);
	lv_obj_set_size(obj, SCREEN_WIDTH-2, SCREEN_HEIGHT-65);
	lv_obj_set_pos(obj, 1, y); 
	lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
	ui_ble_server_panel = obj;
	
	obj = lv_obj_create(ui_ble_screen);
	lv_obj_set_size(obj, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 65);
	lv_obj_set_pos(obj, 1, y); 
	lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
	ui_ble_client_panel = obj;
	
	// server
	x = 10; y = 5;
	obj = ui_create_label(ui_ble_server_panel, ble_get_name(), &lv_font_montserrat_30);
	lv_obj_set_pos(obj, x, y); ui_ble_server_name = obj;
	
	x = SCREEN_WIDTH - button_large_width - gap; y = 5;
	obj = ui_create_button(ui_ble_server_panel, "REQUEST", button_large_width, button_h, 2, &lv_font_montserrat_14, ui_ble_server_updatename_event_cb, NULL);
	lv_obj_set_pos(obj, x, y);
	y += button_h + gap;
	obj = ui_create_button(ui_ble_server_panel, "SEND", button_large_width, button_h, 2, &lv_font_montserrat_14, ui_ble_server_send_event_cb, NULL);	
	lv_obj_set_pos(obj, SCREEN_WIDTH - button_large_width - gap, y);
	y += button_h + gap;
	obj = ui_create_button(ui_ble_server_panel, "ADDRESS", button_large_width, button_h, 2, &lv_font_montserrat_14, ui_ble_server_address_event_cb, NULL);	
	lv_obj_set_pos(obj, SCREEN_WIDTH - button_large_width - gap, y);
	
	y = 5 + button_h + gap;
	x = 10;
	obj = lv_textarea_create(ui_ble_server_panel);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_TEXTAREA_BORDER_COLOR), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
	lv_textarea_set_one_line(obj, true);
	lv_obj_set_size(obj, 250, button_h);
	lv_obj_set_pos(obj, x, y);
	lv_obj_add_event_cb(obj, ui_event_edit_cb, LV_EVENT_ALL, NULL);		
	ui_ble_server_send_text = obj;
	
	
	
	y += button_h + gap;
	obj = ui_create_label(ui_ble_server_panel, "XMIT: ", &lv_font_montserrat_20);
	lv_obj_set_pos(obj, x, y);
	obj = ui_create_label(ui_ble_server_panel, LV_SYMBOL_UP, &lv_font_montserrat_24);
	lv_obj_set_style_text_color(obj, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN);	
	lv_obj_set_pos(obj, x+60, y);
	ui_ble_server_sent_status = obj;
	obj = ui_create_label(ui_ble_server_panel, "0", &lv_font_montserrat_20);
	lv_obj_set_pos(obj, x + 90, y);
	ui_ble_server_sent_total = obj;
	
	x = 200;
	obj = ui_create_label(ui_ble_server_panel, "RCV: ", &lv_font_montserrat_20);
	lv_obj_set_pos(obj, x, y);
	obj = ui_create_label(ui_ble_server_panel, LV_SYMBOL_DOWN, &lv_font_montserrat_24);	
	lv_obj_set_style_text_color(obj, lv_color_hex(UI_BUTTON_DISABLE_BG_COLOR), LV_PART_MAIN);
	lv_obj_set_pos(obj, x + 60, y);
	ui_ble_server_receive_status = obj;
	obj = ui_create_label(ui_ble_server_panel, "0", &lv_font_montserrat_20);
	lv_obj_set_pos(obj, x + 90, y);
	ui_ble_server_receive_total = obj;
	
	y += button_h;
	obj = ui_create_button(ui_ble_server_panel, "DICONNECT", 200, button_h, 3, &lv_font_montserrat_16, ui_ble_disconnect_event_cb, NULL);	
	lv_obj_set_pos(obj, 5, y);
	
	// client 
	x = 0; y = 0;
	obj = ui_create_button(ui_ble_client_panel, "SCAN", button_large_width, button_h, 3, &lv_font_montserrat_16, ui_ble_scan_event_cb, NULL);	
	lv_obj_set_pos(obj, x, y);	ui_ble_btn_scan = obj;
	
	obj = ui_create_button(ui_ble_client_panel, "PARING DEVICE", 240, button_h, 3, &lv_font_montserrat_16, ui_ble_pair_event_cb, NULL);
	ui_change_button_color(obj, UI_BUTTON_DISABLE_BG_COLOR, UI_BUTTON_NORMAL_FG_COLOR);
	lv_obj_set_pos(obj, 220, y); ui_ble_pair_button = obj;
	
	x = 120; 
	obj = lv_spinner_create(ui_ble_client_panel, 1000, 60);
	lv_obj_set_size(obj, 30, 30);	
	lv_obj_set_pos(obj, x, y); ui_ble_spinner_scan = obj;
	lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	
	x = 0; y += button_h + gap;
	ui_ble_device_list = lv_obj_create(ui_ble_client_panel);
	lv_obj_set_size(ui_ble_device_list, 190, 150);
	lv_obj_set_pos(ui_ble_device_list, x, y); 
	lv_obj_set_style_pad_all(ui_ble_device_list, 2, LV_PART_MAIN);
	
	x = 210;
	ui_ble_device_detail = lv_obj_create(ui_ble_client_panel);
	lv_obj_clear_flag(ui_ble_device_detail, LV_OBJ_FLAG_SCROLLABLE); /// Flags
	lv_obj_set_size(ui_ble_device_detail, 250, 200);
	lv_obj_set_pos(ui_ble_device_detail, x, y); 
	lv_obj_set_style_pad_all(ui_ble_device_detail, 2, LV_PART_MAIN);
	
	x = 10; y = 2;	
	obj = ui_create_label(ui_ble_device_detail, "NAME: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, x, y); 
	obj = ui_create_label(ui_ble_device_detail, "", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 100, y); ui_ble_device_name = obj;
	
	
	x = 10; y += 25;	
	obj = ui_create_label(ui_ble_device_detail, "ADDRESS: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, x, y); 
	obj = ui_create_label(ui_ble_device_detail, "", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 100, y); ui_ble_device_address = obj;
	
	y += 25;
	obj = lv_textarea_create(ui_ble_device_detail);
	lv_obj_set_style_border_color(obj, lv_color_hex(UI_TEXTAREA_BORDER_COLOR), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
	lv_textarea_set_one_line(obj, true);
	lv_obj_set_size(obj, 150, button_h);
	lv_obj_set_pos(obj, x, y);
	lv_obj_add_event_cb(obj, ui_event_edit_cb, LV_EVENT_ALL, NULL);	
	
	obj = ui_create_button(ui_ble_device_detail, "SEND", button_large_width, button_h, 3, &lv_font_montserrat_16, ui_ble_send_event_cb, obj);
	lv_obj_set_pos(obj, 160, y);
	
	y += button_h + gap;
	obj = ui_create_label(ui_ble_device_detail, "RECEIVED: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, x, y); 
	obj = ui_create_label(ui_ble_device_detail, "######", &lv_font_montserrat_14);
	lv_obj_set_width(obj, 100);
	lv_obj_set_pos(obj, 100, y);  ui_ble_receive = obj;
	
	y += 25;
	obj = ui_create_label(ui_ble_device_detail, "TOTAL: ", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, x, y);
	obj = ui_create_label(ui_ble_device_detail, "######", &lv_font_montserrat_14);
	lv_obj_set_width(obj, 100);
	lv_obj_set_pos(obj, 100, y); ui_ble_total_received = obj;
	ui_ble_switch_screen(0);
	
	
	ui_ble_create_window_set_address();
	lv_timer_create(ui_ble_timer_handler, 100, NULL);
}

void ui_ble_changed_ble_status(uint8_t status)
{
	if(!ui_initialized) return; // Do nothing util UI is initialized.
	lv_obj_t * label = lv_obj_get_child(ui_ble_btn_scan, 0);
	if (status == BLE_CLIENT_SCANNING)
	{
		lv_obj_clean(ui_ble_device_list);
		selected_device = NULL;
		lv_obj_clear_flag(ui_ble_spinner_scan, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(label, "STOP");
	}
	else
	{
		lv_obj_add_flag(ui_ble_spinner_scan, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(label, "SCAN");
	}
}

void ui_ble_add_device(void* dev) {
	if(!ui_initialized) return; // Do nothing util UI is initialized.
	BleRemoteDevice* device = (BleRemoteDevice*)dev;
	lv_obj_t* obj = lv_obj_create(ui_ble_device_list);
	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE); /// Flags
	lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_size(obj, LV_PCT(100), 45);
	lv_obj_set_pos(obj, 5, (45 + 5) * device->id);
	
	lv_obj_t* label = lv_label_create(obj);
	lv_label_set_recolor(label, true);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_label_set_text_fmt(label, "#c4c4c4 " LV_SYMBOL_BLUETOOTH " %s #", device->device_name);
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
	obj->user_data = (void*)device;
	lv_obj_add_event_cb(obj, ui_ble_event_device_item_cb, LV_EVENT_CLICKED, device);
}

lv_obj_t* ui_ble_get_item_by_device(BleRemoteDevice* dev)
{
	for (uint8_t i = 0; i < lv_obj_get_child_cnt(ui_ble_device_list); i++) {
		lv_obj_t* child = lv_obj_get_child(ui_ble_device_list, i);
		if(child->user_data == dev) {
			return child;
		}
	}
	return NULL;
}

void ui_ble_refresh_devices()
{
	lv_obj_clean(ui_ble_device_list); //remove all device items
	for(uint8_t i = 0; i < ble_client_scaned_device_num; i ++) 
	{
		ui_ble_add_device(&ble_client_remote_device[i]);
	}
}

void ui_ble_set_received_data(BleRemoteDevice* dev)
{
	if(selected_device != dev) return;
	lv_label_set_text(ui_ble_receive, (const char*)dev->last_received_buffer);
	sprintf(ui_temp_string, "%d", (int)dev->total_received);
	lv_label_set_text(ui_ble_total_received, ui_temp_string);
}

void ui_ble_set_servername(char* name)
{
	if (ui_ble_server_name)  lv_label_set_text(ui_ble_server_name, name); 
}