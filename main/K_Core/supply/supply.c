#include "supply.h"
#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"
#include "L_Core/ui/ui.h"
#include "L_Core/ui/ui-newps.h"
#include "L_Core/bluetooth/ble.h"
SUPPLY_OBJ supply_obj;
SUPPLY_CMD supply_send_cmd_que[SUPPLY_CMD_SEND_QUE_SIZE];
uint8_t supply_send_que_head = 0;;
uint8_t supply_send_que_tail = 0;
uint8_t supply_received_wait_countdown = 0;
SUPPLY_STATUS_INFO supply_status_info = { 0 };
bool supply_is_emulator;
uint8_t supply_computer_control_on_485[8] = { 1, 5, 0, 0x0A, 0xFF, 00, 0xAC, 0x38 };
uint8_t supply_computer_control_on_485_responsive[8] = { 1, 5, 0, 0x0A, 0xFF, 00, 0xAC, 0x38 };
uint8_t supply_computer_control_off_485[8] = { 1, 5, 0, 0x0A, 00, 00, 0xED, 0xC8 };
uint8_t supply_computer_control_off_485_responsive[8] = { 1, 5, 0, 0x0A, 00, 00, 0xED, 0xC8 };
uint8_t supply_turn_on_voltage_485[8] = { 0x01, 0x05, 0x00, 0x00, 0xFF, 0x00, 0x8C, 0x3A };
uint8_t supply_turn_on_voltage_485_responsive[8] = { 0x01, 0x05, 0x00, 0x00, 0xFF, 0x00, 0x8C, 0x3A };
uint8_t supply_turn_off_voltage_485[8] = { 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0xCD, 0xCA };
uint8_t supply_turn_off_voltage_485_responsive[8] = { 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0xCD, 0xCA };
uint8_t supply_read_teslaman_status_485[8] = { 0x01, 0x01, 0x00, 0x00, 0x00, 0x0A, 0xBC, 0x0D };
uint8_t supply_upper_auth_high_voltage_switch_responsive[7] = { 0x01, 0x01, 0x02, 0x00, 0x00, 0xB9, 0xFC };
uint8_t supply_computer_open_high_voltage_open_responsive[7] = { 0x01, 0x01, 0x02, 0x0A, 0x01, 0x7E, 0x9C };
uint8_t supply_computer_open_high_voltage_off_responsive[7] = { 0x01, 0x01, 0x02, 0x0A, 0x00, 0xBF, 0x5C };
uint8_t supply_set_voltage_current_485[13] = { 0x01, 0x10, 0x0, 0x0, 0x0, 0x2, 0x4, 0x0f, 0xff, 0x0f, 0xff, 0x85, 0x3b };
uint8_t supply_reset_voltage_current_485[13] = { 0x01, 0x10, 0x0, 0x0, 0x0, 0x2, 0x4, 0x00, 0x00, 0x00, 0x00, 0xF3, 0xAF };
uint8_t supply_set_voltage_current_485_responsive[8] = { 0x01, 0x10, 0x0, 0x0, 0x0, 0x2, 0x41, 0xC8 };
uint8_t supply_turn_on_set_voltage_485[11] = { 0x01, 0x0f, 0x0, 0x0, 0x0, 0x0A, 0x2, 0x02, 0x01, 0x25, 0x98 };
uint8_t supply_turn_off_set_voltage_485[11] = { 0x01, 0x0f, 0x0, 0x0, 0x0, 0x0A, 0x2, 0x02, 0x00, 0xE4, 0x58 };
uint8_t supply_turn_set_voltage_485_responsive[8] = { 0x01, 0x0f, 0x0, 0x0, 0x0, 0x0A, 0xD5, 0xCC };


uint8_t supply_read_voltage_current[8] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B };
uint8_t supply_read_voltage_current_responsive[9] = { 0x01, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x33 };

uint8_t supply_crcarray[256];
uint8_t supply_workarray[256];
uint8_t supply_set_voltage[256];
uint16_t supply_checksum = 0;

uint16_t supply_modbus_checksum(uint8_t* buf, size_t len)
{
	uint16_t crc = 0xFFFF;
	for (int pos = 0; pos < len; pos++)
	{
		crc ^= buf[pos];

		for (int i = 8; i != 0; i--)
		{
			if ((crc & 0x0001) != 0)
			{
				crc >>= 1;
				crc ^= 0xA001;
			}
			else
				crc >>= 1;
		}
	}

	// lo-hi
	//return crc;

	// ..or
	// hi-lo reordered
	return (uint16_t)((crc >> 8) | (crc << 8));
}

void supply_send_packaget_to_supply(uint8_t* buf, size_t len)
{
	// if (!systemconfig.serial2.is_485) return;
	memcpy(supply_workarray, buf, len);
	ushort UpperCrcByteMask = 0xff00;
	uint8_t workbyte = 0;
	ushort calculatedCrcValue = 0;
	int numberofcharacterstochecksum = len - 2;
	if (numberofcharacterstochecksum < 1) return;//to short to make a checksum
	memcpy(supply_crcarray, buf, numberofcharacterstochecksum); //copy to calculation array
	calculatedCrcValue = supply_modbus_checksum(supply_crcarray, numberofcharacterstochecksum); //returns new 2 byte crc
	workbyte = (uint8_t)((calculatedCrcValue & UpperCrcByteMask) >> 8); //get first byte
	supply_workarray[numberofcharacterstochecksum] = workbyte; //plug into array
	numberofcharacterstochecksum++; //point to last byte in array
	supply_workarray[numberofcharacterstochecksum] = (uint8_t)(calculatedCrcValue & 0x00ff);
	supply_checksum = calculatedCrcValue;
	supply_send_cmd_que[supply_send_que_head].len = len;
	// ui_newps_add_command((uint8_t*)supply_workarray, len, false);
	memcpy(supply_send_cmd_que[supply_send_que_head].cmd, supply_workarray, len);
	supply_send_que_head++;
	if (supply_send_que_head >= SUPPLY_CMD_QUE_SIZE) supply_send_que_head = 0;
}

void supply_computer_control_on()
{
	supply_send_packaget_to_supply(supply_computer_control_on_485, sizeof(supply_computer_control_on_485));
}

void supply_computer_control_off()
{
	if (supply_is_emulator)
	{
		supply_status_info.computer_control_onoff = false;
	}
	else 
	{
		supply_send_packaget_to_supply(supply_computer_control_off_485, sizeof(supply_computer_control_off_485));	
	}
}
void supply_read_teslaman_status()
{
	if (!supply_is_emulator)
	{
		supply_send_packaget_to_supply(supply_read_teslaman_status_485, sizeof(supply_read_teslaman_status_485));	
	}
	
}

void supply_set_teslaman_voltage_current(uint16_t voltage, uint16_t current)
{
	if (supply_is_emulator)
	{
		supply_status_info.actual_voltage = voltage;
		supply_status_info.actual_current = current;
		supply_status_info.prog_voltage = voltage;
		supply_status_info.prog_current = current;
	}
	else
	{
		ushort desiredVoltsInKv = (ushort)((float)(voltage * 4096) / SUPPLY_MAX_VOLTAGE);
		ushort desiredCurrentInma = (ushort)((float)current * 4096);
		memcpy(supply_set_voltage, supply_set_voltage_current_485, sizeof(supply_set_voltage_current_485));
	
		//plug in desired voltage
		supply_set_voltage[7] = (uint8_t)((desiredVoltsInKv & 0xff00) >> 8); //high order byte
		supply_set_voltage[8] = (uint8_t)((desiredVoltsInKv & 0xff)); //low order byte
		//plug in desired current
		supply_set_voltage[9] = (uint8_t)((desiredCurrentInma & 0xff00) >> 8); //high order byte
		supply_set_voltage[10] = (uint8_t)((desiredCurrentInma & 0xff)); //low order byte
	
		supply_status_info.prog_voltage = voltage;
		supply_status_info.prog_current = current;
		supply_send_packaget_to_supply(supply_set_voltage, sizeof(supply_set_voltage_current_485));	
	}
}

void supply_reset_voltage_current()
{
	supply_status_info.prog_voltage = 0;
	supply_status_info.prog_current = 0;
	if (supply_is_emulator)
	{
		supply_send_packaget_to_supply(supply_reset_voltage_current_485, sizeof(supply_reset_voltage_current_485));	
	}
}

void supply_turn_off_set_voltage()
{
	if (supply_is_emulator)
	{
		supply_status_info.turn_onoff_voltage = false;
	}
	else
	{
		supply_send_packaget_to_supply(supply_turn_off_set_voltage_485, sizeof(supply_turn_off_set_voltage_485));	
	}
	
}

void supply_turn_on_set_voltage()
{
	if (supply_is_emulator)
	{
		supply_status_info.turn_onoff_voltage = true;
	}
	else
	{
		supply_send_packaget_to_supply(supply_turn_on_set_voltage_485, sizeof(supply_turn_on_set_voltage_485));	
	}
	
}

void supply_turn_off_voltage()
{
	if (supply_is_emulator)
	{
		supply_status_info.turn_onoff_voltage = false;
	}
	else { 
		supply_send_packaget_to_supply(supply_turn_off_voltage_485, sizeof(supply_turn_off_voltage_485));
	}
}

void supply_turn_on_voltage()
{
	if (supply_is_emulator)
	{
		supply_status_info.turn_onoff_voltage = true;
	}
	else {
		supply_send_packaget_to_supply(supply_turn_on_voltage_485, sizeof(supply_turn_off_voltage_485));
	}
}

void supply_responsive_voltage_and_current_status()
{
	uint16_t desiredVoltsInKv = (uint16_t)((float)(supply_status_info.actual_voltage * 4096) / SUPPLY_MAX_VOLTAGE);
	uint16_t desiredCurrentInma = (uint16_t)((float)supply_status_info.actual_current * 4096);
	memcpy(supply_set_voltage, supply_read_voltage_current_responsive, sizeof(supply_read_voltage_current_responsive));
	
	//plug in desired voltage
	supply_set_voltage[4] = (uint8_t)((desiredVoltsInKv & 0xff00) >> 8); //high order byte
	supply_set_voltage[5] = (uint8_t)((desiredVoltsInKv & 0xff)); //low order byte
	//plug in desired current
	supply_set_voltage[6] = (uint8_t)((desiredCurrentInma & 0xff00) >> 8); //high order byte
	supply_set_voltage[7] = (uint8_t)((desiredCurrentInma & 0xff)); //low order byte
	supply_send_packaget_to_supply(supply_set_voltage, sizeof(supply_read_voltage_current_responsive));	
}

void supply_init()
{
	memset(&supply_obj, 0, sizeof(SUPPLY_OBJ)); //reset memory space
	supply_obj.serial = &ComUart2;
}

uint8_t supply_buffer_pos;
void supply_parse_incomming_line()
{
	if (!supply_obj.serial) return;
	if (supply_obj.serial->RxBuffer.Head == supply_obj.serial->RxBuffer.Tail)return;//nothing to 
	char WorkRxChar;
	ComBuffer* SourceBuff = &supply_obj.serial->RxBuffer;
	// we need to parse a command on a slice 
	supply_buffer_pos = 0; 
	char* command = supply_obj.que_command[supply_obj.que_head];
	for (uint16_t i = 0; i < PROCESS_MAX_CHARS_TO_READ_ON_ONE_SLICE; i++)
	{
		if (SourceBuff->Head == SourceBuff->Tail) break;
		WorkRxChar = SourceBuff->buffer[SourceBuff->Tail];
		SourceBuff->Tail++; //point to the next character
		SourceBuff->Tail &= (SourceBuff->Buffer_Size - 1); //circular que with even hex size....
		command[supply_buffer_pos] = WorkRxChar;
		supply_buffer_pos++;
	}
	
	supply_obj.que_head++;
	if (supply_obj.que_head >= SUPPLY_CMD_QUE_SIZE) supply_obj.que_head = 0;
}
uint8_t supply_get_data_size(uint8_t func)
{
	uint8_t len = 0;
	switch (func)
	{
	case 0x05: len = 8; break;
	case 0x0F: len = supply_is_emulator ? 11 : 8; break;
	case 0x10: len = supply_is_emulator ? 13: 8; break;
	case 0x01: len = supply_is_emulator?8: 7; break;
	case 0x03: len = supply_is_emulator?8: 9; break;
	}
	return len;
}

void supply_check_or_incomming_command()
{
	if (supply_received_wait_countdown > 0)
	{
		// it will wait for 5ms
		supply_received_wait_countdown--;
		return;
	}
}
void supply_send_command() {
	if (supply_received_wait_countdown > 0) return;
	if (supply_send_que_head == supply_send_que_tail) return;
	SUPPLY_CMD* supply_cmd = &supply_send_cmd_que[supply_send_que_tail];
	communication_add_buffer_to_serial_buffer(&supply_obj.serial->TxBuffer, (uint8_t*)supply_cmd->cmd, supply_cmd->len);
	ui_newps_add_command((uint8_t*)supply_cmd->cmd, supply_cmd->len, false);
	supply_send_que_tail++;
	if (supply_send_que_tail >= SUPPLY_CMD_QUE_SIZE) supply_send_que_tail = 0;
}

void supply_send_responsive_to_ble(uint8_t* buf, int len)
{
	memset(temp_string, 0, 256);
	temp_string[0] = 'S';
	temp_string[1] = 'P';
	temp_string[2] = '<';
	temp_string[3] = '<';
	char* temp = temp_string + 4;
	for (uint8_t i = 0; i < len; i++)
	{
		sprintf(temp, "%02X ", buf[i]);
		temp += 3;
	}
	temp[0] = '\n';
	communication_add_string_to_ble_buffer(&bleDevice.TxBuffer, temp_string);
}
void supply_process_incomming_command_sequence()
{
	if (supply_obj.que_head == supply_obj.que_tail) return;
	supply_received_wait_countdown = 5;
	char* command = supply_obj.que_command[supply_obj.que_tail];
	supply_obj.que_tail++;
	if (supply_obj.que_tail >= SUPPLY_CMD_QUE_SIZE) supply_obj.que_tail = 0;
	uint8_t id = command[0];
	uint8_t func = command[1];
	
	uint8_t total_size = supply_get_data_size(func);
	if (total_size >= SUPPLY_CMD_MAX_LEN || total_size == 0) return;
	uint16_t crc = (command[total_size -2] << 8) | command[total_size - 1];
	uint16_t calculatedCrcValue = supply_modbus_checksum((uint8_t*)command, total_size - 2); //returns new 2 byte 
	
	if (!supply_is_emulator)
	{
		supply_send_responsive_to_ble((uint8_t*)command, total_size);
	}
	
	if (crc != calculatedCrcValue) {
		ui_newps_add_log("---- INVALID CHECKSUM ----", UI_COLOR_ERROR);
		ui_newps_add_command((uint8_t*)command, total_size, true);
		return; // invalid checksumm
	}
	
	ui_newps_add_command((uint8_t*)command, total_size, true);
	
	switch (func)
	{
	case 0x1: // Read power status
		if (supply_is_emulator)
		{
			if (command[4] == 0x00 && command[5] == 0x0A)
			{
				// Read power status
				// 01 01 00 00 00 0A BC 0D
				if (supply_status_info.computer_control_onoff)
				{
					supply_send_packaget_to_supply(supply_computer_open_high_voltage_open_responsive, sizeof(supply_computer_open_high_voltage_open_responsive));
				}
				else
				{
					supply_send_packaget_to_supply(supply_computer_open_high_voltage_off_responsive, sizeof(supply_computer_open_high_voltage_off_responsive));
				}
			}
		}
		else
		{
			if (command[3] == 0xA && command[4] == 0x0) {
				// computer open _ high voltage open
				supply_status_info.computer_control_onoff = 0;
			}
			else if (command[3] == 0xA && command[4] == 0x1) {
				// computer open _ high voltage open
				supply_status_info.computer_control_onoff = 1; 
			}
			break;	
		}
		break;
		
	case 0x3: // Read power supply voltage and current status
		if (supply_is_emulator)
		{
			supply_responsive_voltage_and_current_status();
		}
		else
		{
			uint16_t voltage = (command[4] << 8) + command[5];
			uint16_t current = (command[6] << 8) + command[7];
			ushort voltsInV = (ushort)((float)(voltage / 4096.0) * SUPPLY_MAX_VOLTAGE);
			ushort currentInma = (ushort)((float)current / 4096.0);
			supply_status_info.actual_voltage = voltsInV;
			supply_status_info.actual_current = currentInma;
		}
		break;
	case 0x5:
		if (supply_is_emulator)
		{
			if (command[4] == 0xff && command[5] == 0x0) supply_status_info.computer_control_onoff = true;
			else if (command[4] == 0x0 && command[5] == 0x0) supply_status_info.computer_control_onoff = false;
			supply_send_packaget_to_supply((uint8_t*)command, total_size);
		}
		break;
	case 0xF:
		if (supply_is_emulator)
		{
			supply_send_packaget_to_supply(supply_turn_set_voltage_485_responsive, sizeof(supply_turn_set_voltage_485_responsive));
		}
		break;
	case 0x10:
		if (supply_is_emulator)
		{
			int v = ((command[7] << 8) + command[8]);
			int c = ((command[9] << 8) + command[10]);
			supply_status_info.actual_voltage = (v * SUPPLY_MAX_VOLTAGE / 4096.0);
			supply_status_info.actual_current = (c / 4096.0);
			supply_status_info.prog_voltage = supply_status_info.actual_voltage;
			supply_status_info.prog_current = supply_status_info.actual_current;
			supply_send_packaget_to_supply(supply_set_voltage_current_485_responsive, sizeof(supply_set_voltage_current_485_responsive));
		}
		break;
	}
}