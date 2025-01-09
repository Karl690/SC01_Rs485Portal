#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "secshelper.h"


//little endian to big endian 
void SwapBytes(void *pv, size_t n)
{
	char *p = (char*)pv;
	size_t lo, hi;
	for (lo = 0, hi = n - 1; hi > lo; lo++, hi--)
	{
		char tmp = p[lo];
		p[lo] = p[hi];
		p[hi] = tmp;
	}
}
#define SWAP(x, n) SwapBytes(&x, n);
char secsstringSendList[SECS_STRING_LIST_MAX_SIZE][SECS_MAX_ROW_SIZE];
char secsstringReceiveList[SECS_STRING_LIST_MAX_SIZE][SECS_MAX_ROW_SIZE];
int ConvertSecsBinaryToStringList(uint8_t* buf, char* stringList)
{
	uint16_t row = 0;
	char* row_string = (char*) (stringList);
	uint8_t s = (uint8_t)(buf[3] % 128);
	uint8_t f = (uint8_t)buf[4];
	uint8_t w =  (buf[3] & 0x80) == 0 ? 0 : 1;
	memset(stringList, 0, SECS_MAX_ROW_SIZE* SECS_STRING_LIST_MAX_SIZE); //reset buffer list
	sprintf(row_string, "S%dF%d %s", s, f, w ? "W" : ""); //for example , S1F1 W
	row++;
	uint16_t len = buf[0];
	uint16_t i = 11;
	uint8_t isList = 0;
	uint16_t list_num = 0;
	uint8_t pad_size = 1;
	uint8_t pad_index = 1;
	char values[SECS_MAX_ROW_SIZE] = {0};
	uint8_t endian[8] = { 0 };
	while (i < len)
	{
		uint8_t format = (uint8_t)(buf[i] & 0xFC);
		i++;
		uint8_t num = buf[i];
		i++;
		if (num > 0)
		{
			row_string = (char*)(stringList + row * SECS_MAX_ROW_SIZE);
			memset(values, 0, SECS_MAX_ROW_SIZE);
			switch (format)
			{
			case SECS_LIST:
				isList = 1;
				list_num = num;
				sprintf(row_string, "%*s<L[%d]", pad_index * pad_size,"", num); 
				pad_index++;
				num = 0;
				break;
			case SECS_Binary: 
				sprintf(row_string, "%*s<B[%d] ", pad_index * pad_size, " ", num);
				for (int k = 0; k < num; k++)
				{
					if (k > 0) strcat(row_string, " ");
					sprintf(values, "0x%X", buf[k + i]);
					strcat(row_string, values);
				}
				strcat(row_string, ">");
				if (isList) list_num--;
				break;
			case SECS_Boolean: 
				sprintf(row_string, "%*s<Boolean[%d] ", pad_index * pad_size, " ", num);
				for (int k = 0; k < num; k++)
				{
					if (k > 0) strcat(row_string, " ");
					sprintf(values, "0x%d", buf[k + i]);
					strcat(row_string, values);
				}
				strcat(row_string, ">");
				if (isList) list_num--;
				break;
			case SECS_ASCII:
				sprintf(row_string, "%*s<A[%d] '", pad_index * pad_size, " ", num);
				for (int k = 0; k < num; k++)
				{
					sprintf(values, "%c", buf[k + i]);
					strcat(row_string, values);
				}
				strcat(row_string, "'>");
				if (isList) list_num--;
				break;
			case SECS_I8:
			case SECS_U8:
				sprintf(row_string, "%*s<%s[%d] ", pad_index * pad_size, " ", format == SECS_I8? "I8":"U8", num/8);
				for (int k = 0; k < num; k+= 8)
				{
					if (k > 0) strcat(row_string, " ");
					memcpy(endian, buf + k + i, 8);
					SWAP(endian, 8); //big to little.
					sprintf(values, "%d", (int)(format == SECS_I8 ? *(int64_t*)endian : *(int64_t*)endian));
					strcat(row_string, values);
				}
				strcat(row_string, ">");
				if (isList) list_num--;
				break;
			case SECS_I4:
			case SECS_U4:
				sprintf(row_string, "%*s<%s[%d] ", pad_index * pad_size, " ", format == SECS_I8 ? "I4" : "U4", num/4);
				for (int k = 0; k < num; k += 4)
				{
					if (k > 0) strcat(row_string, " ");
					memcpy(endian, buf + k + i, 4);
					SWAP(endian, 4); //big to little.
					sprintf(values, "%d", (int)(format == SECS_I4 ? *(int32_t*)endian : *(int32_t*)endian));
					strcat(row_string, values);
				}
				strcat(row_string, ">");
				if (isList) list_num--;
				break;
			case SECS_I2:
			case SECS_U2:
				sprintf(row_string, "%*s<%s[%d] ", pad_index * pad_size, " ", format == SECS_I8 ? "I2" : "U2", num/2);
				for (int k = 0; k < num; k += 2)
				{
					if (k > 0) strcat(row_string, " ");
					memcpy(endian, buf + k + i, 2);
					SWAP(endian, 2); //big to little.
					sprintf(values, "%d", format == SECS_I2 ? *(int16_t*)endian : *(int16_t*)endian);
					strcat(row_string, values);
				}
				strcat(row_string, ">");
				if (isList) list_num--;
				break;
			case SECS_I1:
			case SECS_U1:
				sprintf(row_string, "%*s<%s[%d] ", pad_index * pad_size, " ", format == SECS_I8 ? "I1" : "U1", num);
				for (int k = 0; k < num; k += 1)
				{
					if (k > 0) strcat(row_string, " ");
					sprintf(values, "%d", format == SECS_I1 ? (int8_t)buf[k + i] : (uint8_t)buf[k + i]);
					strcat(row_string, values);
				}
				strcat(row_string, ">");
				if (isList) list_num--;
				break;
			case SECS_F4:
				sprintf(row_string, "%*s<F4[%d] ", pad_index * pad_size, " ", num/4);
				for (int k = 0; k < num; k += 4)
				{
					if (k > 0) strcat(row_string, " ");
					memcpy(endian, buf + k + i, 4);
					SWAP(endian, 4); //big to little.
					sprintf(values, "%d", (int)(format == SECS_I8 ? *(float*)endian : *(float*)endian));
					sprintf(values, "%f", *((float*)&buf[k + i]));
					strcat(row_string, values);
				}
				strcat(row_string, ">");
				if (isList) list_num--;
				break;
			case SECS_F8:
				sprintf(row_string, "%*s<F8[%d] ", pad_index * pad_size, " ", num/8);
				for (int k = 0; k < num; k += 8)
				{
					if (k > 0) strcat(row_string, " ");
					memcpy(endian, buf + k + i, 8);
					SWAP(endian, 8); //big to little.
					sprintf(values, "%d", (int)(format == SECS_I8 ? *(long*)endian : *(long*)endian));
					strcat(row_string, values);
				}
				strcat(row_string, ">");
				if (isList) list_num--;
				break;
			default:
				break;
			}
			row++;
			if (isList && list_num == 0)
			{
				row_string = (char*)(stringList + row * SECS_MAX_ROW_SIZE);
				pad_index--;
				sprintf(row_string, "%*s>.", pad_index * pad_size, " ");
				row++;
			}
		}
		i += num;
	}
	return row;
}