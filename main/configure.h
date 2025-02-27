//#define USE_OPC
#pragma once
#include <stdint.h>

#define SDCARD_MOUNT_POINT "/sd-card"
#define SYSTEM_CONFIG_FILE SDCARD_MOUNT_POINT"/config.ini"
#define AMPLIIFER_CSV_FILE SDCARD_MOUNT_POINT"/amplifier.csv"

#define CMD_MAX_SIZE 50
#define RX_BUF_SIZE  0x200
#define TX_BUF_SIZE  0x200
#define RX_URGENT_BUF_SIZE 0x200	//1k


#define SCREEN_WIDTH	480
#define SCREEN_HEIGHT	320

#define INDICATER_COUNTDOWN 3

typedef struct
{
	uint8_t ssid[32];
	uint8_t password[32];
	uint8_t autoconnect;
	uint8_t status; //1: connected, 0: disconnected
	uint8_t ip[30];
	uint8_t subnet[30];
} WIFI_CONFIG;

typedef struct
{
	uint8_t autostart; 
	uint8_t status; //1: connected, 0: disconnected	
	uint8_t server_enabled;
	uint8_t client_enabled;
} BLUETOOTH_CONFIG;
typedef struct
{
	uint8_t automount; 
	uint8_t status; //1: connected, 0: disconnected	
} SDCARD_CONFIG;

typedef struct
{
	uint8_t username[30]; 
	uint8_t userpassword[30]; 
	uint8_t autostart; 
	uint8_t status; //1: connected, 0: disconnected	
} OPC_CONFIG;
typedef struct 
{
	uint8_t rx_pin;
	uint8_t tx_pin;
	uint32_t baud;
	uint8_t mode; //0: SIMPLE, 1: SECS
	uint8_t is_485;
} SERIAL_CONFIG;

typedef struct
{
	uint8_t timerReload1;
	uint8_t timerReload2;
	uint8_t timerRetry;
} SECS_CONFIG;
typedef struct 
{
	uint8_t initialized;
	WIFI_CONFIG wifi;
	BLUETOOTH_CONFIG bluetooth;
	OPC_CONFIG opc;
	SDCARD_CONFIG sdcard;
	SERIAL_CONFIG serial1;
	SERIAL_CONFIG serial2;
	SECS_CONFIG secs;
	uint8_t ScreenControlEnabled;
	uint8_t server_base_address;
	uint8_t can_address;
}SYSTEMCONFIG;
