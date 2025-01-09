#include <stdint.h>


typedef enum {
	SECS_LIST = 0,
	// 000000 00
	SECS_Binary = 0x20,
	// 001000 00
	SECS_Boolean = 0x24,
	// 001001 00
	SECS_ASCII = 0x40,
	// 010000 00
	SECS_JIS8 = 0x44,
	// 010001 00
	SECS_I8 = 0x60,
	// 011000 00
	SECS_I1 = 0x64,
	// 011001 00
	SECS_I2 = 0x68,
	// 011010 00
	SECS_I4 = 0x70,
	// 011100 00
	SECS_F8 = 0x80,
	// 100000 00
	SECS_F4 = 0x90,
	// 100100 00
	SECS_U8 = 0xA0,
	// 101000 00
	SECS_U1 = 0xA4,
	// 101001 00
	SECS_U2 = 0xA8,
	// 101010 00
	SECS_U4 = 0xB0,
	// 101100 00
}SecsFormat;
//
//typedef struct tagSecsItem
//{
//	SecsFormat Format;
//	uint8_t		Count;
//	void*	RawData;
//	
//}SecsItem;
//
//
//
//
//typedef struct tagSecsMessage
//{
//	uint8_t Stream, Function;
//	uint8_t RepleyExpected;
//	SecsItem* SecsItem;
//	char Name[10];
//	
//}SecsMessage;
#define SECS_STRING_LIST_MAX_SIZE	30
#define SECS_MAX_ROW_SIZE	30
extern char secsstringSendList[SECS_STRING_LIST_MAX_SIZE][SECS_MAX_ROW_SIZE];
extern char secsstringReceiveList[SECS_STRING_LIST_MAX_SIZE][SECS_MAX_ROW_SIZE];
// void ConvertToSecsMessage(uint8_t* buf, SecsMessage* secs);
int ConvertSecsBinaryToStringList(uint8_t* buf, char *stringList);