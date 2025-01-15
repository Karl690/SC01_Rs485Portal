#include "supply.h"
#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"

uint32_t supply_desired_voltage = 0;
uint32_t supply_desired_current = 0;
uint32_t supply_actual_voltage = 0;
uint32_t supply_actual_current = 0;
uint8_t supply_computer_control_on_485[8] = { 1, 5, 0, 0x0A, 0xFF, 00, 0xAC, 0x38 };
uint8_t supply_computer_control_off_485[8] = { 1, 5, 0, 0x0A, 00, 00, 0xED, 0xC8 };
uint8_t supply_turn_on_voltage_485[8] = { 0x01, 0x05, 0x00, 0x00, 0xFF, 0x00, 0x8C, 0x3A };
uint8_t supply_turn_off_voltage_485[8] = { 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0xCD, 0xCA };
uint8_t supply_read_teslaman_status_485[8] = { 0x01, 0x02, 0x00, 0x00, 0x00, 0x08, 0x79, 0xCC };
uint8_t supply_set_voltage_current_485[13] = { 0x01, 0x10, 0x0, 0x0, 0x0, 0x2, 0x4, 0x0f, 0xff, 0x0f, 0xff, 0x85, 0x3b };
uint8_t supply_reset_voltage_current_485[13] = { 0x01, 0x10, 0x0, 0x0, 0x0, 0x2, 0x4, 0x00, 0x00, 0x00, 0x00, 0xF3, 0xAF };
uint8_t supply_turn_on_set_voltage_485[11] = { 0x01, 0x0f, 0x0, 0x0, 0x0, 0x0A, 0x2, 0x02, 0x01, 0x25, 0x98 };
uint8_t supply_turn_off_set_voltage_485[11] = { 0x01, 0x0f, 0x0, 0x0, 0x0, 0x0A, 0x2, 0x02, 0x00, 0xE4, 0x58 };

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
	communication_add_buffer_to_serial_buffer(&ComUart2.TxBuffer, supply_workarray, len);
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
	
	supply_send_packaget_to_supply(supply_set_voltage, sizeof(supply_set_voltage_current_485));
}

void supply_reset_voltage_current()
{
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
	supply_send_packaget_to_supply(supply_turn_off_voltage_485, sizeof(supply_turn_off_voltage_485));
}