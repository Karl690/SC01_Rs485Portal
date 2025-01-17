#include "main.h"
#include "../serial/serial.h"
#include "simple.h"
#include "L_Core/devices/display.h"
#include "L_Core/ui/ui.h"
#include "L_Core/ui/ui-simple.h"
#include "K_Core/amplifier/amplifier.h"
SIMPLE_OBJ simple_obj;
GENERATOR_STATUS simple_generator_status[SIMPLE_GENERATOR_NUM];
uint16_t simple_buffer_pos = 0;
char ui_simple_temp[1024] = { 0 };
uint8_t ui_simple_cmd[10];
uint32_t simple_dump_display_address = 0;

void simple_init()
{
	memset(&simple_obj, 0, sizeof(SIMPLE_OBJ)); //reset memory space
	memset(simple_generator_status, 0, SIMPLE_GENERATOR_NUM* sizeof(SIMPLE_OBJ));
	simple_obj.serial = &ComUart2;
}
void simple_send_command(uint8_t code)
{
	if (systemconfig.serial2.mode != SERIAL2_MODE_SIMPLE) return; //mode:0 SIMPLE, 1: SECS;  	
	
	uint16_t len = 0;
	switch (code)
	{
	case SIMPLE_CMD_PING:
		simple_send_ping();
		break;
	case SIMPLE_CMD_START_LOGGING:
		ui_simple_cmd[0] = 'L'; ui_simple_cmd[1] = '0'; ui_simple_cmd[2] = 0xd; len = 3;
		break;
	case SIMPLE_CMD_STOP_LOGGING:
		ui_simple_cmd[0] = 'l'; ui_simple_cmd[1] = '0'; ui_simple_cmd[2] = 0xd; len = 3;
		break;
	case SIMPLE_CMD_QUERY:
		ui_simple_cmd[0] = 'Q'; ui_simple_cmd[1] = '1'; ui_simple_cmd[2] = 0xd; len = 3;
		break;
	case SIMPLE_CMD_UPDATE_RECIPE:
		ui_simple_cmd[0] = 'U'; ui_simple_cmd[1] = '1'; ui_simple_cmd[2] = 0xd; len = 3;
		break;
	case SIMPLE_CMD_DOWNLOAD_RECIPE:
		ui_simple_cmd[0] = 'D'; ui_simple_cmd[1] = '1'; ui_simple_cmd[2] = 0xd; len = 3;
		break;
	case SIMPLE_CMD_START_PROCESS:
		ui_simple_cmd[0] = 'O'; ui_simple_cmd[1] = '0'; ui_simple_cmd[2] = 0xd; len = 3;
		break;
	case SIMPLE_CMD_CANCLE_PROCESS:
		ui_simple_cmd[0] = 'o'; ui_simple_cmd[1] = '0'; ui_simple_cmd[2] = 0xd; len = 3;
		break;
	case SIMPLE_CMD_IDENTIFY:
		ui_simple_cmd[0] = 'I'; ui_simple_cmd[1] = '0'; ui_simple_cmd[2] = 0xd; len = 3;
		break;
	}
	if (simple_obj.serial)
	{
		communication_add_buffer_to_serial_buffer(&simple_obj.serial->TxBuffer, ui_simple_cmd, len);
		memset(ui_simple_temp, 0, 10);	
		strncpy(ui_simple_temp, (const char*)ui_simple_cmd, len);
		ui_simple_add_log(ui_simple_temp, UI_SEND_COLOR);
	}
}

void ParseIncommingLineToSimpleString()
{
	if (!simple_obj.serial) return;
	if (simple_obj.serial->RxBuffer.Head == simple_obj.serial->RxBuffer.Tail)return;//nothing to 
	char WorkRxChar;
	ComBuffer* SourceBuff = &simple_obj.serial->RxBuffer;
	COMPORT* WorkingComPort = simple_obj.serial;
	char* command = simple_obj.que_commands[simple_obj.que_head];
	for (uint16_t i = 0; i < PROCESS_MAX_CHARS_TO_READ_ON_ONE_SLICE; i++)
	{
		if (SourceBuff->Head == SourceBuff->Tail) break;
		WorkRxChar = SourceBuff->buffer[SourceBuff->Tail];
		SourceBuff->Tail++; //point to the next character
		SourceBuff->Tail &= (SourceBuff->Buffer_Size - 1); //circular que with even hex size....
		if (WorkRxChar  > 0x19 && WorkRxChar <= 0x7F)
		{
			command[simple_buffer_pos] = WorkRxChar;
			simple_buffer_pos++;
		}
		switch (WorkRxChar)
		{
		case PING_REPLY:
			if (!dump_display_sending)
			{
				WorkingComPort->TxAcknowledgeCounter--; //keep track of how far behind we are
			}
			if (WorkingComPort->TxAcknowledgeCounter < 0) WorkingComPort->TxAcknowledgeCounter = 0; //in case of underrun from reset condition
			if (WorkingComPort->pingSent)
			{
				ui_simple_add_char(PING_REPLY, UI_RECEIVE_COLOR);	
				WorkingComPort->pingSent = false;
			}
			break;
		case PING_CHAR:
			WorkingComPort->RxAcknowledgeCounter = 1; // replay 0x6 only at one time.
			WorkingComPort->TxAcknowledgeCounter = 0;
			// display_reset_capture_buffer(); // reset capture buffer.
			// dump_display_sending = false;
			ui_simple_add_char(PING_CHAR, UI_RECEIVE_COLOR);	
			ui_simple_add_char(PING_REPLY, UI_SEND_COLOR);		
			break;
		case CR_CHAR:
		case CMD_END_CHAR:
			if (simple_buffer_pos == 0) break;
			command[simple_buffer_pos] = 0;
			//simple_obj.head++;
			simple_buffer_pos = 0;
			simple_obj.que_head++;
			if (simple_obj.que_head >= SIMPLE_CMD_QUE_SIZE) simple_obj.que_head = 0;
			break;
		}
	}
}

void simple_responsive_identifier(char* command)
{
	
}

void simple_send_ping()
{
	communication_add_char_to_serial_buffer(&simple_obj.serial->TxBuffer, PING_CHAR);	
	simple_obj.serial->pingSent = true;
	ui_simple_add_line("Send ping 0x7", UI_SEND_COLOR, false);
}
void simple_parse_generator_status(char* command)
{
	char szTemp[100] = { 0 };
	strcpy(szTemp, command);
	char* token = strtok(szTemp, ",");
	bool is_parsed = false;
	GENERATOR_STATUS g = { 0};
	for (int i = 0; i < 11; i++)
	{	
		if (!token) break;
		switch (i)
		{
		case 0:
			g.ampUnit = atoi(token);
			break;
		case 1:
			g.channel = atoi(token);
			if (g.channel >= 8) return;
			break;
		case 2:
			g.freq1 = atoi(token);
			break;
		case 3:
			g.tc6 = atoi(token);
			break;
		case 4:
			g.freq2= atoi(token);
			break;
		case 5:
			g.power1 = atoi(token);
			break;
		case 6:
			g.power2 = atoi(token);
			break;
		case 7:
			g.bathTemp = atoi(token);
			break;
		case 8:
			g.tc1 = atoi(token);
			break;
		case 9:
			g.tc2 = atoi(token);
			break;
		case 10:
			g.status = atoi(token);
			break;
		}
	}
	if (is_parsed)
	{
		memcpy(&simple_generator_status[g.channel], &g, sizeof(GENERATOR_STATUS));
	}
}

void simple_send_dump_screen()
{
	if (!dump_display_sending) {
		if (dump_display_waiting > 1) dump_display_waiting--;
		else if(dump_display_waiting == 1)
		{
			uart_write_bytes(ComUart2.uart_id, "DC\n", 3);	//Complete = C
			ui_simple_add_log("DC\n", UI_SEND_COLOR);
			dump_display_waiting--;
		}
		return;
	}
	
	// wait a little before transfering data.
	if (dump_display_waiting > 0)
	{
		dump_display_waiting--;
		return;
	}
	uint32_t address = simple_dump_display_address;
	if (address >= display_compress_buffer_size) {
		dump_display_waiting = WAITING_VALUE;
		dump_display_sending = false;
		return;
	}
	
	size_t bytes = SCREEN_DUMP_SIZE;
	if (simple_dump_display_address + SCREEN_DUMP_SIZE >= display_compress_buffer_size)
	{
		bytes = display_compress_buffer_size - simple_dump_display_address;
	}
	uart_write_bytes(ComUart2.uart_id, display_snapshot_compress_buffer + address, bytes);
	simple_dump_display_address += bytes;
	sprintf(temp_string, "%d/%d bytes sent", (int)simple_dump_display_address, display_compress_buffer_size);
	ui_simple_add_log(temp_string, UI_SEND_COLOR);
}

void simple_parse_command()
{
	if (simple_obj.que_head == simple_obj.que_tail) return;
	char* command = simple_obj.que_commands[simple_obj.que_tail];
	simple_obj.que_tail++;
	if (simple_obj.que_tail >= SIMPLE_CMD_QUE_SIZE) simple_obj.que_tail = 0; //0xf
	ui_simple_add_log(command, UI_RECEIVE_COLOR);
	uint32_t index = 0;
	char mark = command[0];
	switch (mark)
	{
	case 'R':
		simple_dump_display_address = 0;
		dump_display_sending = false;
		dump_display_waiting = 0;
		uart_write_bytes(ComUart2.uart_id, "DS\n", 3); // Start
		if (display_dump_buffer())
		{
			sprintf(temp_string, "DN %d\n", display_compress_buffer_size); //Display Number = DN
			uart_write_bytes(ComUart2.uart_id, temp_string, strlen(temp_string));
			dump_display_waiting = WAITING_VALUE;
			dump_display_sending = true;
		}
		else
		{
			uart_write_bytes(ComUart2.uart_id, "DE\n", 3); // ERROR
		}
		break;
	case 'r':
		dump_display_waiting = 0;
		dump_display_sending = false;
		uart_write_bytes(ComUart2.uart_id, "DU\n", 3); // Display stop by user
		break;
	case 'L': // Start sending Q string
		index = atoi(command + 1);
		amplifier_set_logging(index, true);
		break;
	case 'l': // Stop sending Q string
		index = atoi(command + 1);
		amplifier_set_logging(index, false);
		break;
	case 'I': simple_responsive_identifier(command + 1);
		break;
	case 'Q': simple_parse_generator_status(command + 1);
		break;
	case 'O': // start process
		index = atoi(command + 1);
		if (index > MAX_CHANNELS) return; // invalid
		sprintf(ui_simple_temp, "M801 I660 T%d\n", (int)(index == 0 ? 255 : (index - 1) * 6));
		communication_add_string_to_serial_buffer(&MasterCommPort->TxBuffer, ui_simple_temp); 
		break;
	case 'o': // stop
		index = atoi(command + 1);
		if (index > MAX_CHANNELS) return; // invalid
		sprintf(ui_simple_temp, "M801 I661 T%d\n", (int)(index == 0 ? 255 : (index - 1) * 6));
		communication_add_string_to_serial_buffer(&MasterCommPort->TxBuffer, ui_simple_temp); 
		break;
	case 'X':
		sprintf(ui_simple_temp, "M801 I661 T255\n");
		communication_add_string_to_serial_buffer(&MasterCommPort->TxBuffer, ui_simple_temp); 
		break;
	}
}