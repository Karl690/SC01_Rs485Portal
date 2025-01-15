#pragma  once
#include "main.h"
#define SUPPLY_MAX_VOLTAGE 30000
#define SUPPLY_MIN_VOLTAGE 0
extern uint32_t supply_desired_voltage;
extern uint32_t supply_desired_current;
extern uint32_t supply_actual_voltage;
extern uint32_t supply_actual_current;
extern uint16_t supply_checksum;
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