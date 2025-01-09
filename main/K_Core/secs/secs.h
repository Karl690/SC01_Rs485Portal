#pragma once
#pragma once
#include "main.h"
#include "../serial/serial.h"
#define SECS_BUFFER_LENGTH 300

#define SECS_DEVICE_ID 1

#define EOT			0x04	//End of Transmission. we use this as the first byte of var pairs 's block
#define FOV			0x0A	//First byte of Var	pair
#define EOV			0x0D	//End byte of var pair
#define FOC			0xFF	//Formatted byte of Charactor

#define ACK		(uint8_t)6
#define NAK		(uint8_t)21
#define ENQ		(uint8_t)5

#define __MAX(a, b) (a > b? a: b)
#define __MIN(a, b) (a < b? a: b)

typedef enum
{
	S1F1  = 101,
	S1F2  = 102,
	S1F3  = 103,
	S1F4  = 104,
	S1F5  = 105,
	S1F6  = 106,
	S2F19 = 219,
	S2F20 = 220,
	S2F21 = 221,
	S2F22 = 222,
	S7F1  = 701,
	S7F2  = 702,
	S7F3  = 703,
	S7F4  = 704,
	S7F5  = 705,
	S7F6  = 706,
	S9F1  = 901,
	S9F3  = 903,
	S9F5  = 905,
	S9F7  = 907
}SECSCOMMAND;

typedef struct tagSecsMsgPacket
{
	uint8_t DataLength;
	uint8_t	Id;
	uint8_t Stream;
	uint8_t Function;
	uint8_t ReceiveWbit;
	uint8_t SystemByte1;
	uint8_t SystemByte2;
	uint8_t SystemByte3;
	uint8_t SystemByte4;
	uint16_t CheckSum;
	uint8_t RawData[SECS_BUFFER_LENGTH];
}SecsMsgPacket;


typedef struct
{
	COMPORT* serial;
}SECS_OBJ;

extern SECS_OBJ secs_obj;
extern uint16_t secstimer1;
extern uint16_t secstimer2;
extern uint16_t secstimer3;
extern uint16_t secstimer5;
extern uint16_t secstimer6;
extern uint16_t secstimer7; //autoconnect in 3 seconds
extern uint16_t CHECKSUMCALCULATED;
extern uint32_t CheckedSumReceived;
extern uint16_t secsmessagelength;
extern uint16_t checksum_passed;
extern uint16_t numberofretriesleft;
extern uint16_t defaultnumberofretriesleft;
extern uint16_t secssendfail;

extern uint32_t SecsReceivedMessageTotalErrorNum;
extern uint16_t secssendpass;
extern uint16_t systembytes;
extern uint16_t systembyte34;
extern uint16_t receiveid;
extern uint8_t secs1_flag;
extern char ReceivedSecsCmd[10];
extern char SentSecsCmd[10];
extern uint8_t secs_receive_buffer[SECS_BUFFER_LENGTH];
extern uint8_t secs_transmit_buffer[SECS_BUFFER_LENGTH];

extern uint32_t secs_rx_num;
extern uint32_t secs_tx_num;


extern uint8_t s1f1message[13];
extern uint8_t s1f2message[41];
extern uint8_t s1f5message[16];
extern uint8_t s1f6message[26];
extern uint8_t s1f6VerteqSRDmessage[52];
extern uint8_t s2f19message[16];
extern uint8_t s2f20message[13];
extern uint8_t s2f20VerteqSRDmessage[16];
extern uint8_t s2f21message[16];
extern uint8_t s2f22message[16];
extern uint8_t s7f1message[21];
extern uint8_t s7f2message[16];
extern uint8_t s7f3message[26];
extern uint8_t s7f4message[];
extern uint8_t s7f5message[18];
extern uint8_t s7f6message[24];
extern uint8_t s9f1message[25];
extern uint8_t s9f3message[25];
extern uint8_t s9f5message[25];
extern uint8_t s9f7message[25];
extern uint8_t s9f9message[];
extern uint8_t s9f11message[];
extern uint8_t s9f13message[];


extern int secs_transfer_rows;
extern int secs_received_rows;
extern bool secs_is_recevied;
extern bool secs_is_transfered;
extern SecsMsgPacket LastReceivedMessage;
extern SecsMsgPacket LastSentMessage;
void secs_init();
void SendUartSecsString(char* stringToSend);
void ParseIncommingLineToSecsString();

void SecsTimers(void);

void PrcessSecsReceivedMessage();

void SendSecsCommand(uint8_t* secsbuf, uint16_t size);
void VerteqSRD_idme(uint8_t* secssendbuffer);
void VerteqMeg_idme(uint8_t* secssendbuffer);
void systemMe(uint8_t* secssendbuffer);
