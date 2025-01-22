#pragma  once
#include "main.h"
#include "K_Core/serial/serial.h"
#define SUPPLY_MAX_VOLTAGE		30000
#define SUPPLY_MAX_KVOLTAGE		30
#define SUPPLY_MIN_VOLTAGE		0
#define SUPPLY_MIN_KVOLTAGE		0

#define SUPPLY_CMD_QUE_SIZE 0xf
#define SUPPLY_CMD_MAX_LEN 40

#define SUPPLY_CMD_SEND_QUE_SIZE 20

typedef struct
{
	uint8_t computer_control_onoff;
	uint32_t prog_voltage;
	uint32_t prog_current;
	uint32_t actual_voltage;
	uint32_t actual_current;
	uint8_t turn_onoff_voltage;
}SUPPLY_STATUS_INFO;
typedef struct
{
	COMPORT* serial;
	uint16_t que_tail;
	uint16_t que_head;
	char que_command[SUPPLY_CMD_QUE_SIZE][SUPPLY_CMD_MAX_LEN];
	uint8_t waitingOfResponsive;
}SUPPLY_OBJ;

typedef struct
{
	char cmd[20];
	uint8_t len;
}SUPPLY_CMD;
extern SUPPLY_STATUS_INFO supply_status_info;
extern uint16_t supply_checksum;
extern bool supply_is_emulator;
uint16_t supply_modbus_checksum(uint8_t* buf, size_t len);

void supply_send_packaget_to_supply(uint8_t* buf, size_t len);

void supply_computer_control_on();
void supply_computer_control_off();
void supply_read_teslaman_status();
void supply_set_teslaman_voltage_current(uint16_t voltage, uint16_t current);

void supply_reset_voltage_current();
void supply_turn_off_set_voltage();
void supply_turn_on_set_voltage();

void supply_turn_off_voltage();
void supply_turn_on_voltage();

void supply_init();
void supply_parse_incomming_line();
void supply_process_incomming_command_sequence();
void supply_check_or_incomming_command();
void supply_send_command();