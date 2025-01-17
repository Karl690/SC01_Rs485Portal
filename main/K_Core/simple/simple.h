#pragma once

#define SIMPLE_CMD_MAX_LEN 50
#define SIMPLE_CMD_QUE_SIZE 0x10

#define SIMPLE_GENERATOR_NUM 8
#define WAITING_VALUE 20

#define SCREEN_DUMP_SIZE 2048
#define SCREEN_SIZE SCREEN_HEIGHT * SCREEN_WIDTH * 2
enum
{
	SIMPLE_CMD_PING = 50,
	SIMPLE_CMD_START_LOGGING,
	SIMPLE_CMD_STOP_LOGGING,
	SIMPLE_CMD_QUERY,
	SIMPLE_CMD_UPDATE_RECIPE,
	SIMPLE_CMD_DOWNLOAD_RECIPE,
	SIMPLE_CMD_START_PROCESS,
	SIMPLE_CMD_CANCLE_PROCESS,
	SIMPLE_CMD_IDENTIFY,
};



typedef struct
{
	COMPORT* serial;
	uint16_t que_tail;
	uint16_t que_head;
	char que_commands[SIMPLE_CMD_QUE_SIZE][SIMPLE_CMD_MAX_LEN];
}SIMPLE_OBJ;

typedef struct
{
	uint8_t ampUnit;  //0
	uint8_t channel;  // 1
	uint16_t freq1;   // 2
	uint16_t freq2;		// 4
	uint16_t power1;	// 5
	uint16_t power2;	// 6
	uint16_t tc1;		// 8
	uint16_t tc2;		// 9
	uint16_t tc6;		// 3
	uint16_t bathTemp;	// 7
	uint8_t status;		// 10
}GENERATOR_STATUS;

extern SIMPLE_OBJ simple_obj;
void simple_init();
void simple_send_command(uint8_t code);
void simple_send_ping();
void simple_parse_command();
void simple_send_dump_screen();
void ParseIncommingLineToSimpleString();