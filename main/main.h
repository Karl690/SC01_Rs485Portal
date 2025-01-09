#pragma once
#include "configure.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <inttypes.h>
#include <string.h>
#include <esp_chip_info.h>
#include <esp_event.h>
#include <esp_flash.h>
#include <esp_ota_ops.h>
#include <soc/rtc.h>
//#include <fmt/core.h>
//#include <fmt/format.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"


#include "L_Core/devices/conf_WT32SCO1-Plus.h"

typedef enum
{
	RUN_NORMAL,
	//not server and client
	RUN_BLE_SERVER,
	// server mode
	RUN_BLE_CLIENT,
	//client mode
}RUN_MODE;

extern RUN_MODE run_mode;
extern const char *TAG;
extern SYSTEMCONFIG systemconfig;
extern bool dump_display_sending;
extern uint32_t dump_display_waiting;
extern bool IsInitialized;
extern char temp_string[256];
bool load_configuration();
bool save_configuration();

char *trim(char *s);
int random_int(int min, int max);