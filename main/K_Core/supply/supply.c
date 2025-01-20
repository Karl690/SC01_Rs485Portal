#include "supply.h"
#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"
#include "L_Core/ui/ui.h"
#include "L_Core/ui/ui-newps.h"

SUPPLY_OBJ supply_obj;
SUPPLY_STATUS_INFO supply_status_info;

uint8_t supply_computer_control_on_485[8] = { 1, 5, 0, 0x0A, 0xFF, 00, 0xAC, 0x38 };
uint8_t supply_computer_control_off_485[8] = { 1, 5, 0, 0x0A, 00, 00, 0xED, 0xC8 };
uint8_t supply_turn_on_voltage_485[8] = { 0x01, 0x05, 0x00, 0x00, 0xFF, 0x00, 0x8C, 0x3A };
uint8_t supply_turn_off_voltage_485[8] = { 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0xCD, 0xCA };
//uint8_t supply_read_teslaman_status_485[8] = { 0x01, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x5, 0x6 };
uint8_t supply_read_teslaman_status_485[8] = { 0x01, 0x01, 0x00, 0x00, 0x00, 0x0A, 0xBC, 0x0D };
uint8_t supply_set_voltage_current_485[13] = { 0x01, 0x10, 0x0, 0x0, 0x0, 0x2, 0x4, 0x0f, 0xff, 0x0f, 0xff, 0x85, 0x3b };
uint8_t supply_reset_voltage_current_485[13] = { 0x01, 0x10, 0x0, 0x0, 0x0, 0x2, 0x4, 0x00, 0x00, 0x00, 0x00, 0xF3, 0xAF };
uint8_t supply_turn_on_set_voltage_485[11] = { 0x01, 0x0f, 0x0, 0x0, 0x0, 0x0A, 0x2, 0x02, 0x01, 0x25, 0x98 };
uint8_t supply_turn_off_set_voltage_485[11] = { 0x01, 0x0f, 0x0, 0x0, 0x0, 0x0A, 0x2, 0x02, 0x00, 0xE4, 0x58 };

uint8_t supply_read_voltage_current[8] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B };

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
	communication_add_buffer_to_serial_buffer(&supply_obj.serial->TxBuffer, supply_workarray, len);
	supply_obj.waitingOfResponsive = 1;
	ui_newps_add_command(supply_workarray, len, false);
}

void supply_computer_control_on()
{
	supply_send_packaget_to_supply(supply_computer_control_on_485, sizeof(supply_computer_control_on_485));
}

void supply_computer_control_off()
{
	supply_send_packaget_to_supply(supply_computer_control_off_485, sizeof(supply_computer_control_off_485));
}
void supply_read_teslaman_status()
{
	supply_send_packaget_to_supply(supply_read_teslaman_status_485, sizeof(supply_read_teslaman_status_485));
}

void supply_set_teslaman_voltage_current(uint16_t voltage, uint16_t current)
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

void supply_reset_voltage_current()
{
	supply_status_info.prog_voltage = 0;
	supply_status_info.prog_current = 0;
	supply_send_packaget_to_supply(supply_reset_voltage_current_485, sizeof(supply_reset_voltage_current_485));
}

void supply_turn_off_set_voltage()
{
	supply_send_packaget_to_supply(supply_turn_off_set_voltage_485, sizeof(supply_turn_off_set_voltage_485));
}

void supply_turn_on_set_voltage()
{
	supply_send_packaget_to_supply(supply_turn_on_set_voltage_485, sizeof(supply_turn_on_set_voltage_485));
}

void supply_turn_off_voltage()
{
	supply_send_packaget_to_supply(supply_turn_off_voltage_485, sizeof(supply_turn_off_voltage_485));
}

void supply_turn_on_voltage()
{
	supply_send_packaget_to_supply(supply_turn_on_voltage_485, sizeof(supply_turn_off_voltage_485));
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
	if (supply_obj.que_head >= SUPPLY_CMD_MAX_LEN) supply_obj.que_head = 0;
}
uint8_t supply_get_data_size(uint8_t func)
{
	uint8_t len = 0;
	switch (func)
	{
	case 0x05: len = 8; break;
	case 0x0F: len = 8; break;
	case 0x10: len = 8; break;
	case 0x01: len = 7; break;
	case 0x03: len = 9; break;
	}
	return len;
}
void supply_process_command_sequence()
{
	if (supply_obj.que_head == supply_obj.que_tail) return;
	char* command = supply_obj.que_command[supply_obj.que_tail];
	supply_obj.que_tail++;
	if (supply_obj.que_tail >= SUPPLY_CMD_MAX_LEN) supply_obj.que_tail = 0;
	uint8_t id = command[0];
	uint8_t func = command[1];
	
	uint8_t total_size = supply_get_data_size(func);
	if (total_size >= SUPPLY_CMD_MAX_LEN || total_size == 0) return;
	uint16_t crc = (command[total_size -2] << 8) | command[total_size - 1];
	uint16_t calculatedCrcValue = supply_modbus_checksum((uint8_t*)command, total_size - 2); //returns new 2 byte 
	if (crc != calculatedCrcValue) {
		ui_newps_add_log("---- INVALID CHECKSUM ----", UI_COLOR_ERROR);
		ui_newps_add_command((uint8_t*)command, total_size, true);
		return; // invalid checksumm
	}
	
	ui_newps_add_command((uint8_t*)command, total_size, true);
	
	switch (func)
	{
	case 0x1: // Read power status
		if (command[4] == 0xA && command[5] == 0x0) supply_status_info.high_voltage_onoff = false;
		else if (command[4] == 0xA && command[5] == 0x1) supply_status_info.high_voltage_onoff = true;
		break;
	case 0x3: // Read power supply voltage and current status
		{
			uint16_t voltage = (command[4] << 8) + command[5];
			uint16_t current = (command[6] << 8) + command[7];
			ushort voltsInV = (ushort)((float)(voltage / 4096.0) * SUPPLY_MAX_VOLTAGE);
			ushort currentInma = (ushort)((float)current / 4096.0);
			supply_status_info.actual_voltage = voltsInV;
			supply_status_info.actual_current = currentInma;
			break;
		}
	}
}