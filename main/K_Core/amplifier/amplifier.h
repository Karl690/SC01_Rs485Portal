#pragma once
#include "main.h"
#define MAX_CHANNELS 6
typedef struct {
	//programmable variables
//M801 A B C D I=0  in order
	uint16_t				ProgPower1; //P- Programmed Power from SC01 or LSharp
	uint16_t				ProgFrequency1; //F-Programd Frequency from SC01 or LSharp
	uint16_t				ProgWobble1; //F-Programd Frequency from SC01 or LSharp
	uint16_t				FreqLock; //enables process to adjust the DAC to get deisred frequency
//M801 A B C D I=4  in order
	uint16_t				ProgPower2; //P- Programmed Power from SC01 or LSharp
	uint16_t				ProgFrequency2; //F-Programd Frequency from SC01 or LSharp
	uint16_t				ProgWobble2; //F-Programd Frequency from SC01 or LSharp
	uint16_t				spareDummy1; //for future expansion

//M801 A B C D I=8  in order
	uint16_t				DesiredTimeSec; //S- count down value 
	uint16_t				TuneFlag; //autotunning state
	uint16_t				spareDummy2; //for future expansion
	uint16_t				spareDummy3; //for future expansion
	
			// Tuning parameters
//M801 A B C D I=12  in order

	uint16_t				TunePower1; //Q-power level in percent during tunning 
	uint16_t				TuneFreqStart1; //A-lower freq to start the sweep for tunning
	uint16_t				TuneFreqStop1; //B-upper freq for tunning sweep
	uint16_t				TuneVelocity1; //V-how fast the freq control dac counts up during tunning
//M801 A B C D I=16  in order
	uint16_t				TunePower2; //Q-power level in percent during tunning 
	uint16_t				TuneFreqStart2; //A-lower freq to start the sweep for tunning
	uint16_t				TuneFreqStop2; //B-upper freq for tunning sweep
	uint16_t				TuneVelocity2; //V-how fast the freq control dac counts up during tunning
//group 6 dummy variables for future expansion
	uint16_t				min_Ok_Power; //for future expansion
	uint16_t				spareDummy5; //for future expansion
	uint16_t				spareDummy6; //for future expansion
	uint16_t				spareDummy7; //for future expansion
}AmplifierChannelSettingStruct;
typedef struct
{
	//--------------------------reporting variables
	uint16_t				ActualTimeSec; //non zero if power is running
	uint16_t				DacValue; //D-DAC value
	uint16_t				ActualFrequency; // Actual frequency by stored TIM1 or TIM3.
	uint16_t				ForwordPower; //actual forward power sensor reading
	uint16_t				ReflectedPower; //reflected power sensor reading
	uint16_t				GateDriveVoltage; //gate drive voltage comes from A/D input

	// Hardware paramters
	uint16_t				HvRegualtorFrequency; //defines the pulse rate for the High voltage reg ulator, fixed pulse width, variable freq
	uint16_t				HvRegualtorMaxFrequency; //X-defines the pulse rate for the High voltage reg ulator, fixed pulse width, variable freq
	uint16_t				BridgeGateVotageSelect; //used to set gate drive voltage
	uint16_t				TuneMaxPeakFrequency; //upper freq for tunning sweep
	uint16_t				State; //upper freq for tunning sweep
	uint16_t				DataSendToHostRequest;
	uint16_t				ArrayPluggedIn;
}AmplifierChannelVariableStruct;

typedef struct
{
	// real time control parameters
	uint16_t CanAddress; //11,12,13, 21, 21, 23 Can address(s)
	AmplifierChannelSettingStruct settings;
	AmplifierChannelVariableStruct vars;
}AmplifierChannelStruct;
typedef struct {
	AmplifierChannelStruct RF_Channels[MAX_CHANNELS]; //make space for maximum channels
	//can base address
	uint16_t				NumberOfChannels; //default is 3, RF channels are 2 , and I/O channel is 1
	uint8_t					Address; // BaseAddress + dif switch address
	uint16_t				Duty; //controls the CSP-3000 or UHP-1500 voltage output 
} AmplifierStruct;


typedef struct
{
	uint8_t Channel;
	uint16_t PROGRAMMED_POWER1;
	uint16_t PROCESS_TIME;
	uint16_t UHP_Voltage_Control_Duty;
	uint16_t PROCESS_TIMER;
	uint8_t ProcessMode;
}AmpProcessInfoStruct;

extern uint16_t amplifier_processmode; //0: Process, 1: Programming mode
extern bool amplifier_logging[MAX_CHANNELS];
extern char amplifier_major_step[10];

extern AmplifierStruct amplifier;
extern AmpProcessInfoStruct ampProcessInfo;
void amplifier_init();
void amplifier_channel_init_with_default_value(uint16_t channelOffset);

void amplifier_build_status_string();
void amplifier_set_logging(uint8_t address, bool status);