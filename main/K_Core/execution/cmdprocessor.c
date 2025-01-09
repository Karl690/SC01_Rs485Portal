
#include "cmdprocessor.h"
#include "bcode.h"
#include "L_Core/bluetooth/ble.h"
#include "L_Core/ui/ui.h"
#include "L_Core/ui/ui-comm.h"
#include "L_Core/ui/ui-pct.h"
#include "L_Core/ui/ui-pct01.h"
#include "L_Core/ui/ui-bluetooth.h"
#include "L_Core/ui/ui-plot.h"
#include "L_Core/bluetooth/ble.h"
#include "K_Core/amplifier/amplifier.h"
#include "K_Core/secs/secs.h"
uint32_t cmd_NextCommandInsertionPointer = 1;
uint32_t cmd_CurrentPointer = 1;
uint32_t cmd_CommandsInQue = 0;
char cmd_CommandsInQueBuffer[SIZE_OF_COMMAND_QUEUE][MAX_COMMAND_LEN];
uint32_t cmd_start_freq = 9300;
uint32_t cmd_stop_freq = 9800;
uint32_t cmd_freq_inc = 100;
GMBCOMMAND cmd_Command;
char cmdproc_temp[256];
char lineIndexBuffer[5];
CMD_REPORT_INFO cmd_report_que[0x10];
uint32_t cmd_report_tail = 0;
uint32_t cmd_report_head = 0;
bool cmd_tuning = false;
bool cmd_sending_log = false;
FILE* cmd_log_fp = NULL;
void cmd_sequener()
{
    if(!cmd_CommandsInQue) return;						//no commands to proces, so return
    char* cmd = &cmd_CommandsInQueBuffer[cmd_CurrentPointer][0];
	int len = strlen(cmd);
	if (cmd[len - 1] == '\n') cmd[len - 1] = '\0'; //remove  '\n'
	parseLineCommandData(cmd);
	ui_comm_add_log(cmd, UI_RECEIVE_COLOR);
    cmd_CurrentPointer ++;
    if(cmd_CurrentPointer >= SIZE_OF_COMMAND_QUEUE) cmd_CurrentPointer = 1;
	cmd_CommandsInQue--;
}
void parseLineCommandData(char* cmd)
{
	int index = -1;
	char* temp = strstr(cmd, "*");// cmd.IndexOf("*");
	if (!temp) return;
	index = temp - cmd;
	if (index < 1) return;
	int firstPos = -1;
	char command = ' '; 
	for (int i = index-1; i >= 0; i--)
	{
		switch (cmd[i])
		{
		case 'L':
		case 'l':
		case 'B':
		case 'b':
		case 'S':
		case 'A':
		case 'R':
		case 'P':
		case 'X':
		case 'D':
			command = cmd[i];
			firstPos = i;
			break;
		}	
	}
	
	if (firstPos == -1) return;
	char szindx[5] = { 0 };
	strncpy(szindx, cmd + firstPos + 1, index - firstPos -1); // string s = cmd.Substring(firstPos + 1, index - firstPos - 1);
	int idx = atoi(szindx);
	switch (command)
	{
	case 'A':
		parseActionCommand(cmd + 2); //move 2 characters to skip the "A*"
		break;
	case 'R':
		parseReportCommand(cmd + 2); //move 2 characters to skip the "R*"
		break;
	case 'P':
		parseProcessInfoCommand(cmd + 2); //move 2 characters to skip the "R*"
		break;
	case 'L': //L#=string
		//parseLineTextCommand(cmd + 2);
		if (idx < 10) ui_pct_update_label_text(idx, cmd + index + 1);
		else ui_pct01_update_label_text(idx - 10, cmd+index+1);
		break;
	case 'l': //l#=color
		if (idx < 10) ui_pct_update_label_color(idx, cmd+index+1);
		else ui_pct01_update_label_color(idx - 10, cmd+index+1);
		break;
	case 'B': //B#=string
		ui_pct_update_button_text(idx, cmd + index + 1);
		ui_pct01_update_button_text(idx, cmd + index + 1);
		break;
	case 'b': //b#=color
		ui_pct_update_button_color(idx, cmd + index + 1);
		ui_pct01_update_button_color(idx, cmd + index + 1);
		break;
	case 'S': //update the BLE device address
		//if (idx != 99) break;
		updateBleAddress(cmd +2);
		break;
	case 'X':
		processBleScreenAction(idx, atoi(cmd + index + 1));
		break;
	case 'D': //D#
		processBleSendLogFile(idx);
		break;
	}	
}

void parseG6Command(char* cmd)
{
	//G6 B#\n
	//char code = cmd[3];
	int value = atoi(cmd + 4); //go to place of Number
	switch (value)
	{
	case 18: //transform comm  screen
		ui_transform_screen(SCREEN_COMM);	
		break;
	case 19: //transform home screen
		ui_transform_screen(SCREEN_HOME);
		break;
	case 99: //G6 S99 ; this is a request to update the ble name from 407		
		break;
	default:
		communication_tx_commandline(MasterCommPort, cmd);
		break;
	}
}


//parse action string
// A*T#,StartFq,StopFqB, FreqInc
// A*T255
void parseActionCommand(char* cmd)
{
	strcpy(cmdproc_temp, cmd);
	const char s[2] = " ";
	char* token =  strtok(cmdproc_temp, s);
	int index = 0;
	int tArg = 0;
	while (token != NULL) {
		switch (index)
		{
		case 0:
			if (token[0] != 'T') return;
			tArg = atoi(token + 1);
			if (tArg == 255)cmd_tuning = false;
			break;
		case 1:
			if (tArg == 254) cmd_start_freq = atoi(token);
			break;
		case 2:
			if (tArg == 254) cmd_stop_freq = atoi(token);
			break;
		case 3:
			if (tArg == 254) {
				cmd_freq_inc = atoi(token);
				cmd_tuning = true;
				ui_plot_clear();
			}
			break;
		default:
			break;
		}
		index++;
		token = strtok(NULL, s);
	}
	//ui_plot_button_status(cmd_turning);	
}

//parse report string
// R*T# ActualFreq Dac ForwardPwr RefectedPwr HvFreq GateDrvVolt State\n
void parseReportCommand(char* cmd)
{
	strcpy(cmdproc_temp, cmd);
	const char s[2] = " ";
	char* token =  strtok(cmdproc_temp, s);
	bool is_validate = false;
	CMD_REPORT_INFO temp_info = { 0 };
	int index = 0;
	while (token != NULL) {
		switch (index)
		{
		case 0:
			if (token[0] != 'T') return;
			temp_info.chanel = atoi(token + 1); //remove T and convert to integer
			if (temp_info.chanel >= UI_PLOT_CHANNEL_NUM) return;
			break;
		case 1:
			temp_info.actrual_freq = atoi(token);
			break;
		case 2:
			temp_info.dac = atoi(token);
			break;
		case 3:
			temp_info.forward_power = atoi(token);
			break;
		case 4:
			temp_info.refected_power = atoi(token);
			break;
		case 5:
			temp_info.array_plugin = atoi(token);
			break;
		case 6:
			temp_info.process_mode = atoi(token);
			break;
		case 7:
			strcpy(temp_info.major_step, token);
			is_validate = true;
			break;
		default:
			break;
		}
		index++;
		token = strtok(NULL, s);
	}
	if (is_validate)
	{
		CMD_REPORT_INFO* info = &cmd_report_que[cmd_report_head];
		
		memcpy(info, &temp_info, sizeof(CMD_REPORT_INFO));
		amplifier.RF_Channels[info->chanel].CanAddress = info->chanel;
		amplifier.RF_Channels[info->chanel].vars.DacValue = info->dac;
		amplifier.RF_Channels[info->chanel].vars.ForwordPower = info->forward_power;
		amplifier.RF_Channels[info->chanel].vars.ReflectedPower = info->refected_power;
		amplifier.RF_Channels[info->chanel].vars.ArrayPluggedIn = info->array_plugin;
		amplifier_processmode = info->process_mode;
		strcpy(amplifier_major_step, info->major_step);
		cmd_report_head++;
		cmd_report_head &= 0xf;
	}
}

//parse process information string
// P*T# PROGRAMMED_POWER1 PROCESS_TIME UHP_Voltage_Control_Duty PROCESS_TIMER ProcessMode
// A*T255
void parseProcessInfoCommand(char* cmd)
{
	strcpy(cmdproc_temp, cmd);
	const char s[2] = " ";
	char* token =  strtok(cmdproc_temp, s);
	bool is_passed = false;
	
	int index = 0;
	int tArg = 0;
	while (token != NULL) {
		switch (index)
		{
		case 0:
			if (token[0] != 'T') return;
			tArg = atoi(token + 1);
			if (tArg >= MAX_CHANNELS) return;
			ampProcessInfo.Channel = tArg;
			break;
		case 1:
			ampProcessInfo.PROGRAMMED_POWER1 = atoi(token);
			break;
		case 2:
			ampProcessInfo.PROCESS_TIME = atoi(token);
			break;
		case 3:
			ampProcessInfo.UHP_Voltage_Control_Duty = atoi(token);
			break;
		case 4:
			ampProcessInfo.PROCESS_TIMER = atoi(token);
			break;
		case 5:
			ampProcessInfo.ProcessMode = atoi(token);
			is_passed = true;
			break;
		}
		index++;
		token = strtok(NULL, s);
	}
	if (systemconfig.serial2.mode == SERIAL2_MODE_SECS)
	{
		//SECS mode
		if (is_passed) 
		{
			// send s1f6 command.
			s1f6message[15] = ampProcessInfo.Channel; // channel
			s1f6message[16] = (uint8_t)ampProcessInfo.ProcessMode; // Status of Meg407
			s1f6message[17] = (uint8_t)0; // Error of Meg407
			s1f6message[20] = (uint8_t)(ampProcessInfo.PROGRAMMED_POWER1 >> 8) & 0x00ff; // high bit of Power
			s1f6message[21] = (uint8_t)(ampProcessInfo.PROGRAMMED_POWER1 & 0x00ff); // high bit of Power
			s1f6message[22] = (uint8_t)(ampProcessInfo.PROCESS_TIME >> 8) & 0x00ff; // high bit of Time
			s1f6message[23] = (uint8_t)(ampProcessInfo.PROCESS_TIME & 0x00ff); // high bit of Time	
		}
		else
		{
			// send s1f6 command.
			s1f6message[15] = ampProcessInfo.Channel; // channel
			s1f6message[16] = (uint8_t)ampProcessInfo.ProcessMode; // Status of Meg407
			s1f6message[17] = (uint8_t)128; // Error of Meg407
		}
		SendSecsCommand(s1f6message, sizeof(s1f6message));	
	}
}
int findOfSpaceOnString(char* cmd)
{
	int len = strlen(cmd);
	int index = -1;
	for (int i = 0; i < len; i++)
	{
		if (cmd[i] == ' ') {
			index = i;
			break;
		}
	}
	return index;
}
void parseLineTextCommand(char* cmd)
{
	int index = findOfSpaceOnString(cmd);
	if (index == -1) return;
	char szT[5] = { 0 };
	
	strncpy(szT, cmd, index);
	int idx = atoi(szT);
	if (idx < 10) ui_pct_update_label_text(idx, cmd + index + 1);
	else ui_pct01_update_label_text(idx - 10, cmd + index +1);
}

void parseLineColorCommand(char* cmd)
{
	int index = findOfSpaceOnString(cmd);
	if (index == -1) return;
	char szT[5] = { 0 };
	
	strncpy(szT, cmd, index);
	int idx = atoi(szT);
	if (idx < 10) ui_pct_update_label_color(idx, cmd + index + 1);
	else ui_pct01_update_label_color(idx - 10, cmd + index + 1);
}

void parseButtonTextCommand(char* cmd)
{
	int index = findOfSpaceOnString(cmd);
	if (index == -1) return;
	char szT[5] = { 0 };
	
	strncpy(szT, cmd, index);
	int idx = atoi(szT);
	if (idx < 10) ui_pct_update_button_text(idx, cmd + index + 1);
	else ui_pct01_update_button_text(idx - 10, cmd + index + 1);
}
void parseButtonColorCommand(char* cmd)
{
	int index = findOfSpaceOnString(cmd);
	if (index == -1) return;
	char szT[5] = { 0 };
	
	strncpy(szT, cmd, index);
	int idx = atoi(szT);
	if (idx < 10) ui_pct_update_button_color(idx, cmd + index + 1);
	else ui_pct01_update_button_color(idx - 10, cmd + index + 1);
}

void updateBleAddress(char* cmd)
{
	int index = findOfSpaceOnString(cmd);
	if (index == -1) return;
	char szT[5] = { 0 };
	
	strncpy(szT, cmd, index);
	int idx = atoi(szT);
	if (idx != 99) return;
	int address = atoi(cmd + index + 1);
	ble_update_name(address);
	systemconfig.can_address =(uint16_t) address;//update for sending messages to 407
	save_configuration();
}

void processBleScreenAction(uint8_t screen_id, uint8_t button_id)
{
	ui_request_update = true;
	ui_request_screen_id = screen_id;
	ui_request_button_id = button_id;
}

void cmd_transfer_log_file_task()
{
	if(!cmd_sending_log) return;
	if (fgets(temp_string, 256, cmd_log_fp) != NULL)
	{
		// Remove the newline character if present
		temp_string[strcspn(temp_string, "\n")] = '\0'; // Replace newline with null-terminator
		ble_server_send_data((uint8_t*)temp_string, strlen(temp_string));
		vTaskDelay(10 / portTICK_PERIOD_MS); // Delay for 1000ms (1 second)
	}
	else
	{
		fclose(cmd_log_fp);	
		cmd_log_fp = NULL;
		cmd_sending_log = false;
	}
	
}
void processBleSendLogFile(uint8_t action)
{
	sprintf(ui_temp_string, SDCARD_MOUNT_POINT "/TuneHistory.txt");
	
	switch (action)
	{
	case 0: // send log file
		cmd_sending_log = false;
		
		cmd_log_fp = fopen(ui_temp_string, "r");
		if (!cmd_log_fp) {
			strcpy(temp_string, "There is no log file.\n");
			ble_server_send_data((uint8_t*)temp_string, strlen(temp_string));
			return;
		}
		cmd_sending_log = true;
		break;
	case 1:
		remove(ui_temp_string);	
		strcpy(temp_string, "log file is removed.\n");
		ble_server_send_data((uint8_t*)temp_string, strlen(temp_string));
		break;
	}
	
}