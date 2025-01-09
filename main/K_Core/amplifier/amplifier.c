#include "amplifier.h"

#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"
#include "L_Core/ui/ui.h"
#include "L_Core/ui/ui-simple.h"
#include "K_Core/simple/simple.h"

AmplifierStruct amplifier = { 0 };
AmpProcessInfoStruct ampProcessInfo;
uint16_t amplifier_processmode = 0; //0: Process, 1: Programming mode
char amplifier_major_step[10] = { 0 };

char amplifier_temp_string[256] = { 0};
uint8_t amplifier_channle_index = 0;

bool amplifier_logging[MAX_CHANNELS];
void amplifier_init()
{
	amplifier.NumberOfChannels = 2;
	
	for (uint16_t i = 0; i < MAX_CHANNELS; i++)
	{
		amplifier_channel_init_with_default_value(i);
		amplifier_logging[i] = true;
	}
}

void amplifier_channel_init_with_default_value(uint16_t channelOffset)
{
	amplifier.RF_Channels[channelOffset].CanAddress = channelOffset; //11,12,13, 21, 21, 23 Can address(s)
	amplifier.RF_Channels[channelOffset].settings.DesiredTimeSec = 10; //S- count down value 
	amplifier.RF_Channels[channelOffset].vars.ActualTimeSec = 0; //non zero if power is running
	amplifier.RF_Channels[channelOffset].vars.DacValue = 100; //D-DAC value
	amplifier.RF_Channels[channelOffset].settings.FreqLock = 1; //enables process to adjust the DAC to get deisred frequency
	amplifier.RF_Channels[channelOffset].settings.ProgPower1 = 10; //P- Programmed Power from SC01 or LSharp
	amplifier.RF_Channels[channelOffset].settings.ProgFrequency1 = 9510; //F-Programd Frequency from SC01 or LSharp
	amplifier.RF_Channels[channelOffset].settings.ProgWobble1 = 10; //F-Programd Frequency from SC01 or LSharp
	amplifier.RF_Channels[channelOffset].settings.ProgPower2 = 20; //P- Programmed Power from SC01 or LSharp
	amplifier.RF_Channels[channelOffset].settings.ProgFrequency2 = 7521; //F-Programd Frequency from SC01 or LSharp
	amplifier.RF_Channels[channelOffset].settings.ProgWobble2 = 10; //F-Programd Frequency from SC01 or LSharp
	
	amplifier.RF_Channels[channelOffset].vars.ActualFrequency = 0; // Actual frequency by stored TIM1 or TIM3.
	amplifier.RF_Channels[channelOffset].vars.ForwordPower = 0; //actual forward power sensor reading
	amplifier.RF_Channels[channelOffset].vars.ReflectedPower = 0; //reflected power sensor reading
	amplifier.RF_Channels[channelOffset].settings.TuneFlag = 0; //upper freq for tunning sweep
	
	// Hardware paramters
	amplifier.RF_Channels[channelOffset].vars.HvRegualtorFrequency = 0; //defines the pulse rate for the High voltage reg ulator, fixed pulse width, variable freq
	amplifier.RF_Channels[channelOffset].vars.HvRegualtorMaxFrequency = 0; //X-defines the pulse rate for the High voltage reg ulator, fixed pulse width, variable freq
	amplifier.RF_Channels[channelOffset].vars.BridgeGateVotageSelect = 0; //used to set gate drive voltage
	amplifier.RF_Channels[channelOffset].vars.GateDriveVoltage = 0; //gate drive voltage comes from A/D input
	// Tuning parameters
	amplifier.RF_Channels[channelOffset].settings.TunePower1 = 20; //Q-power level in percent during tunning 
	amplifier.RF_Channels[channelOffset].settings.TuneFreqStart1 = 9200; //A-lower freq to start the sweep for tunning
	amplifier.RF_Channels[channelOffset].settings.TuneFreqStop1 = 9900; //B-upper freq for tunning sweep
	amplifier.RF_Channels[channelOffset].settings.TuneVelocity1 = 5; //V-how fast the freq control dac counts up during tunning
	amplifier.RF_Channels[channelOffset].settings.TunePower2 = 10; //Q-power level in percent during tunning 
	amplifier.RF_Channels[channelOffset].settings.TuneFreqStart2 = 700; //A-lower freq to start the sweep for tunning
	amplifier.RF_Channels[channelOffset].settings.TuneFreqStop2 = 800; //B-upper freq for tunning sweep
	amplifier.RF_Channels[channelOffset].settings.TuneVelocity2 = 5; //V-how fast the freq control dac counts up during tunning
	amplifier.RF_Channels[channelOffset].vars.TuneMaxPeakFrequency = 0; //upper freq for tunning sweep
	amplifier.RF_Channels[channelOffset].vars.State = 0; //upper freq for tunning sweep
	amplifier.RF_Channels[channelOffset].vars.DataSendToHostRequest = 0;
}


void amplifier_build_status_string()
{
	if (dump_display_sending || (!dump_display_sending && dump_display_waiting > 0)) return;
	if (systemconfig.serial2.mode != SERIAL2_MODE_SIMPLE) return; // nothing on not Simple mode
	if (!amplifier_logging[amplifier_channle_index]) return; 
	sprintf(amplifier_temp_string,
		"Q%d %d %d %d %d %d %d %s\n",
		amplifier.RF_Channels[amplifier_channle_index].CanAddress,
		amplifier.RF_Channels[amplifier_channle_index].vars.ActualFrequency,
		amplifier.RF_Channels[amplifier_channle_index].vars.DacValue,
		amplifier.RF_Channels[amplifier_channle_index].vars.ForwordPower,
		amplifier.RF_Channels[amplifier_channle_index].vars.ReflectedPower,
		amplifier.RF_Channels[amplifier_channle_index].vars.ArrayPluggedIn,
		amplifier_processmode,//change to megstate,
		amplifier_major_step); //dummy
	ui_simple_add_log(amplifier_temp_string, UI_SEND_COLOR);
	// communication_add_string_to_serial_buffer(&ComUart2.TxBuffer, amplifier_temp_string);
	amplifier_channle_index++;
	if (amplifier_channle_index >= MAX_CHANNELS) amplifier_channle_index = 0;
}

void amplifier_set_logging(uint8_t address, bool status)
{
	if (address == 0)
	{	
		// if 0, all channel set 
		for (uint8_t i = 0; i < MAX_CHANNELS; i++)
		{
			amplifier_logging[i] = status;
		}	
	}
	else
	{
		if (address - 1 > MAX_CHANNELS) return;
		amplifier_logging[address - 1] = status;
	}
}