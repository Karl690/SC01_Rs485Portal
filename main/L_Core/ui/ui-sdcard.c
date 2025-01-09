#include "ui.h"
#include "ui-sdcard.h"
#include <dirent.h> 
#include <stdio.h>

#include "../sd-card/sd-card.h"

lv_obj_t* ui_sdcard_screen;
lv_obj_t* ui_sdcard_folders_panel;
lv_obj_t* ui_sdcard_files_panel;
lv_obj_t* ui_sdcard_path;
lv_obj_t* ui_sdcard_switch_mount;

char current_sdcard_path[MAXNAMLEN + 1] = SDCARD_MOUNT_POINT;
void event_goto_directory_cb(lv_event_t* e)
{
	char* dir_name = (char*)lv_event_get_user_data(e);
	//(char*)obj->user_data;
	if (!dir_name) return;
	char new_path[MAXNAMLEN + MAXNAMLEN] = { 0 };
	if (!strcmp(dir_name, UP_DIRECTORY))
	{	
		int16_t i = 0;
		for (i = strlen(current_sdcard_path); i >= 0; i--)
		{
			if (current_sdcard_path[i] == '/') break;			
		}
		if (i > 0) {
			strncpy(new_path, current_sdcard_path, i);
			ui_sdcard_load_directory(new_path);
		}
	}
	else
	{
		sprintf(new_path, "%s/%s", current_sdcard_path, dir_name);
		ui_sdcard_load_directory(new_path);
	}
}

void ui_sdcard_clear()
{
	lv_label_set_text_fmt(ui_sdcard_path, "PATH: unmounted SD card");
	lv_obj_clean(ui_sdcard_folders_panel);
	lv_obj_clean(ui_sdcard_files_panel);
}
void ui_sdcard_switch_event_cb(lv_event_t* e)
{
	bool status = lv_obj_has_state(ui_sdcard_switch_mount, LV_STATE_CHECKED);
	if (status)
	{
		if (!systemconfig.sdcard.status) systemconfig.sdcard.status = sdcard_mount();
		if (!systemconfig.sdcard.status) ui_show_messagebox(MESSAGEBOX_ERROR, (const char*)"Falied mount sdcard.", 2000);
	}
	else
	{
		if (systemconfig.sdcard.status) sdcard_umount();
		systemconfig.sdcard.status = false;
	}
	if (systemconfig.sdcard.status) {
		lv_obj_add_state(ui_sdcard_switch_mount, LV_STATE_CHECKED);
		ui_sdcard_load_directory(SDCARD_MOUNT_POINT);
	}
	else {
		lv_obj_clear_state(ui_sdcard_switch_mount, LV_STATE_CHECKED);
		ui_sdcard_clear();
	}
}
void ui_sdcard_screen_init(void)	
{
	ui_sdcard_screen = ui_create_screen();	
	ui_create_pct_title(ui_sdcard_screen, SCREEN_SDCARD);
	
	lv_obj_t* obj = ui_create_label(ui_sdcard_screen, "SD CARD", &lv_font_montserrat_20);	
	lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 5);
	
	obj = lv_switch_create(ui_sdcard_screen);
	lv_obj_add_state(obj, systemconfig.sdcard.status);
	lv_obj_set_pos(obj, 10, 5);
	lv_obj_add_event_cb(obj, ui_sdcard_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
	ui_sdcard_switch_mount = obj;
	
	obj = ui_create_label(ui_sdcard_screen, "PATH: /sd-card", &lv_font_montserrat_14);
	lv_obj_set_pos(obj, 10, 45);
	lv_obj_set_width(obj, 380);
	lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
	ui_sdcard_path = obj;
	
	ui_sdcard_folders_panel = lv_obj_create(ui_sdcard_screen);
	lv_obj_set_style_pad_all(ui_sdcard_folders_panel, 2, LV_PART_MAIN);
	lv_obj_set_size(ui_sdcard_folders_panel, 210, 245);
	lv_obj_set_pos(ui_sdcard_folders_panel, 3, 75);
	
	ui_sdcard_files_panel = lv_obj_create(ui_sdcard_screen);
	lv_obj_set_style_pad_all(ui_sdcard_files_panel, 2, LV_PART_MAIN);
	lv_obj_set_size(ui_sdcard_files_panel, 261, 245);
	lv_obj_set_pos(ui_sdcard_files_panel, 216, 75);
}

void ui_sdcard_add_directory(const char* path, uint16_t index)
{
	lv_obj_t* obj = lv_obj_create(ui_sdcard_folders_panel);
	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE); /// Flags
	lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_size(obj, LV_PCT(100), 30);
	lv_obj_set_pos(obj, 5, (30 + 5) * index);
	
	lv_obj_t* label = lv_label_create(obj);
	lv_label_set_text_fmt(label, LV_SYMBOL_DIRECTORY " %s", path);
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
	char* dir_path = (char*)malloc(MAXNAMLEN);
	strcpy(dir_path, path);
	obj->user_data = (void*)dir_path;
	lv_obj_add_event_cb(obj, event_goto_directory_cb, LV_EVENT_CLICKED, (void*)dir_path);
}


void ui_sdcard_add_file(const char* path, uint16_t index)
{
	lv_obj_t* obj = lv_obj_create(ui_sdcard_files_panel);
	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE); /// Flags
	lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(obj, 2, LV_PART_MAIN);
	lv_obj_set_size(obj, LV_PCT(100), 30);
	lv_obj_set_pos(obj, 5, (30 + 5) * index);
	
	lv_obj_t* label = lv_label_create(obj);
	lv_label_set_text_fmt(label, LV_SYMBOL_FILE " %s", path);
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
	obj->user_data = (void*)path;
}

// Comparison function for qsort to sort by name
int compare(const void *a, const void *b) {
	struct dirent *entryA = *(struct dirent **)a;
	struct dirent *entryB = *(struct dirent **)b;
	return strcmp(entryA->d_name, entryB->d_name);
}

void ui_sdcard_load_directory(const char* path)
{
	strcpy(current_sdcard_path, path);
	lv_label_set_text_fmt(ui_sdcard_path, "PATH: %s", current_sdcard_path);
	lv_obj_clean(ui_sdcard_folders_panel);
	lv_obj_clean(ui_sdcard_files_panel);
			
	// Open the directory
	DIR *dir = opendir(path);
	if (!dir) {
		perror("opendir");
		return;
	}
	size_t entryCount = 0;
	
	uint16_t cnt_folder = 0;
	uint16_t cnt_file = 0;
	ui_sdcard_add_directory("..", cnt_folder);
	cnt_folder++;
	
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type != DT_DIR) // if the type is not directory just print it with blue color
		{
			ui_sdcard_add_file(entry->d_name, cnt_file);
			cnt_file++;
		}
		else if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) // if it is a directory 
		{	
			ui_sdcard_add_directory(entry->d_name, cnt_folder);
			cnt_folder++;
		}
	}

	closedir(dir);
}


