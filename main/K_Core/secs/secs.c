#include "secs.h"
#include "../serial/serial.h"
#include "secshelper.h"
#include "K_Core/amplifier/amplifier.h"
#include "RevisionHistory.h"
SECS_OBJ secs_obj;
int secs_transfer_rows = 0;
int secs_received_rows = 0;
bool secs_is_recevied = false;
bool secs_is_transfered = false;
uint8_t secs_request_channel = 0;
void secs_init()
{
	memset(&secs_obj, 0, sizeof(SECS_OBJ));
	secs_obj.serial = &ComUart2;
}
void SendUartSecsString(char* stringToSend)
{
	if (systemconfig.serial2.mode != SERIAL2_MODE_SECS) return; //mode:0 SIMPLE, 1: SECS;
	communication_add_string_to_serial_buffer(&secs_obj.serial->TxBuffer, stringToSend);
}



/////////////////////////////////////////////////////////////////////
uint8_t SecsMessageWaiting = 0;
//secs variables

uint16_t secstimer1 = 15;
uint16_t secstimer2 = 15;
uint16_t secstimer3 = 15;
uint16_t secstimer5 = 10;
uint16_t secstimer6 = 20;
uint16_t secstimer7 = 20; //autoconnect in 3 seconds
uint16_t secs1bytesleft = 0;
uint16_t secsRecieveQuePointer1 = 0;
uint16_t secsRecieveQuePointer2 = 0;
uint8_t secs1_flag = 0;
uint16_t secstimer1default = 50;
uint16_t secstimer2default = 50;
uint16_t secsSendQuePointerHead = 0;
uint16_t SecsSendQuePointerTail = 0;
uint8_t SecsSendQue[8][1000] = { 0 };

uint8_t secs_receive_buffer[SECS_BUFFER_LENGTH] = { 0 };
uint8_t secs_transmit_buffer[SECS_BUFFER_LENGTH] = { 0 };

uint16_t CHECKSUMCALCULATED = 0;
uint16_t secsmessagelength = 0;
uint16_t checksum_passed = 0;
uint16_t numberofretriesleft = 0;
uint16_t defaultnumberofretriesleft = 5;
uint16_t secssendfail = 0;
uint16_t secssendpass = 0;
uint16_t systembytes = 0;
uint16_t systembyte34 = 0;
uint16_t receiveid = 0;
uint16_t my_device_id = 1;
//uint8_t SecsRetrys_ReloadValue = 3; //in c# project it is on 50ms timer
//uint8_t Secs1Timer1_ReLoadValue = 3;
//uint8_t Secs1Timer2_ReLoadValue = 5;

uint8_t ReceivedSecsMessageUpdateReadyFlag = 0;
uint16_t SentSecsMessageUpdateReadyFlag = 0;
uint16_t LastCharacterRecieved = 0;
uint16_t LastCharacterRead = 0;

uint8_t Receivebuffer[4096] = { 0 };

uint32_t SecsReceivedMessageTotalErrorNum = 0;
uint32_t SecsReceivedMessageTimeoutErrorNum = 0;
uint32_t SecsReceivedMessageCheckSumErrorNum = 0;
uint32_t SecsReceivedMessageParsingErrorNum = 0;
uint32_t secs1receivepointer = 0;
uint32_t secs1charactersreceived = 0;
uint32_t checksumfailed = 0;
uint32_t CheckedSumReceived = 0;
uint32_t SecsSendMessageTimeoutErrorNum = 0;
uint32_t SecsSendMessageTotalErrorNum = 0;
SecsMsgPacket CurrentSecsMessage = { 0 };
SecsMsgPacket LastReceivedMessage = { 0 };
SecsMsgPacket LastSentMessage = { 0 };
char ReceivedSecsCmd[10] = { 0 };
char SentSecsCmd[10] = { 0 };

uint32_t secs_rx_num = 0;
uint32_t secs_tx_num = 0;

uint8_t ParseSecsMessage(uint8_t* RawData)
{
	uint16_t len = RawData[0] + 3;
	if (len < 10) return 0;
	if (len >= SECS_BUFFER_LENGTH) return 0;
	memcpy(CurrentSecsMessage.RawData, RawData, len);
	CurrentSecsMessage.DataLength = len;
	CurrentSecsMessage.Id = RawData[2];
	CurrentSecsMessage.Stream = (uint8_t)(RawData[3] % 128); //stream
	CurrentSecsMessage.ReceiveWbit = (uint8_t)(RawData[3] & 0x80); //reply requested bit
	CurrentSecsMessage.Function = RawData[4]; //function
	//[5] block being transmitted, we only suport last block  80
	//[6] 1
	CurrentSecsMessage.SystemByte1 = RawData[7]; //used to identify this exact message to prevent duplicate processing
	CurrentSecsMessage.SystemByte2 = RawData[8];
	CurrentSecsMessage.SystemByte3 = RawData[9];
	CurrentSecsMessage.SystemByte4 = RawData[10];
	CurrentSecsMessage.CheckSum = (RawData[len - 2]) + ((RawData[len - 1]) * 256);
	//plug in CHECKSUM here
	return 1;
}
void parse_send_message()
{
	if (ParseSecsMessage(secs_transmit_buffer))
	{
		sprintf(SentSecsCmd, "S%dF%d", CurrentSecsMessage.Stream, CurrentSecsMessage.Function);
		memcpy(&LastSentMessage, &CurrentSecsMessage, sizeof(SecsMsgPacket)); //save the latest send message 
	 	
		secs_is_transfered = true;
		SentSecsMessageUpdateReadyFlag = 1;
		secs_tx_num++;
	}
}
void TestForSecsMessageReadyToSend()
{
	//this was modified to only work with secs1, secshsms is implemented by the secs4net library

	if (secs1_flag > 0) return;//cant start something new if we are not finished with the last one

	if (secsSendQuePointerHead == SecsSendQuePointerTail) return;//dont send anything if nothing is waiting to go
	uint16_t lengthOfMessage = SecsSendQue[SecsSendQuePointerTail][0];
	memcpy(secs_transmit_buffer, &SecsSendQue[SecsSendQuePointerTail][2], lengthOfMessage);
	
	SecsSendQuePointerTail++; //move pointer to next que in the array for the next message
	SecsSendQuePointerTail &= 7; //only 8 entrys before spinning in the circular que
	
	parse_send_message(); //document the message that is being sent stream, funct, etc.          
	numberofretriesleft = systemconfig.secs.timerRetry; // defaultnumberofretriesleft;//set the number of retries
	secs1_flag = 10;

	secstimer1 = systemconfig.secs.timerReload1 * 20; // secstimer1default;//set the character timeout timer
}

uint16_t SecsCharWaiting()
{
	return (secs_obj.serial->RxBuffer.Head - secs_obj.serial->RxBuffer.Tail);//if non zero result char is ready
}
uint8_t GetLastChar()
{
	if (secs_obj.serial == NULL) return 0;//someone forgot to assign the comport alias
	if (SecsCharWaiting() == 0) return 0;//no chars waiting
	uint8_t RawRxChar = secs_obj.serial->RxBuffer.buffer[secs_obj.serial->RxBuffer.Tail];
	secs_obj.serial->RxBuffer.Tail++;
	secs_obj.serial->RxBuffer.Tail &= (secs_obj.serial->RxBuffer.Buffer_Size - 1);
	return RawRxChar;
}

void secs1_idle()
{
	//check for any charcters waiting, if no then out of here
	//if (LastCharacterRecieved == LastCharacterRead) return;   //if (nextcharacter2 == lastcharacter2) return;
	if (!secs_obj.serial) return;
	if (SecsCharWaiting() == 0) return;//no chars waiting
	//we do have a character
	uint8_t temp = GetLastChar();
	//is ita enq, a start secs1 request
	
	if (temp == ENQ)
	{
		communication_add_char_to_serial_buffer(&secs_obj.serial->TxBuffer, EOT); //sendToSecsSerialPort(EOT);
		secs1_flag = 1;
		secstimer2 = systemconfig.secs.timerReload2 * 20; // secstimer2default;//set the reply timeout timer 
	}
}

/// <summary>
/// this state will check timer2 and if it goes to zero will cancel
/// this atemp at secs
/// otherwise it will try to read a character which should represent
/// the actual length of the message to be received.
/// </summary>
void waiting_for_length()
{
	if (secstimer2 == 0)
	{
		secs1_flag = 0; //reset the attemp flag if not good

		communication_add_char_to_serial_buffer(&secs_obj.serial->TxBuffer, NAK); //sendToSecsSerialPort(Nak);
		
		SecsReceivedMessageTotalErrorNum++;
		SecsReceivedMessageTimeoutErrorNum++;
		//reportError("Waiting for Length byte TimeOut");
		return;         //if you found timer2 is zero we have failed
	}
	if (SecsCharWaiting() == 0) return;//no chars waiting
	uint8_t temp2 = GetLastChar();
	secs_receive_buffer[0] = temp2; //store the length for future use
	secs1bytesleft = temp2; //number of bytes in string
	secs1bytesleft += 2; //add 2 bytes for the checsum
	secs1receivepointer = 1; //set the pointer to second position because we already put the length byte i [0]
	secstimer1 = systemconfig.secs.timerReload1 * 20; // secstimer1default;//set the character timeout timer
	secs1charactersreceived = 1; //set the number of characters recievd to 1
	secs1_flag = 2; //point to next state
}

uint8_t checksumreceivecheck()
{
	int messagelength = secs_receive_buffer[0];
	secsmessagelength = messagelength;

	uint16_t checksum = 0;
	for (int a = 1; a < messagelength + 1; a++)
	{
		checksum += secs_receive_buffer[a]; //add up the next character
	}
	int rcvdchecksum = (secs_receive_buffer[messagelength + 2]);
	rcvdchecksum += ((secs_receive_buffer[messagelength + 1]) * 256);
	CheckedSumReceived = rcvdchecksum;

	CHECKSUMCALCULATED = checksum; //update the global variables for diag
	
	//UpdateSecsData?.Invoke(); // Update CheckSUM, checkSum falied, passed data.
	ReceivedSecsMessageUpdateReadyFlag = 1;
	return checksum == rcvdchecksum;
}

void read_me_a_character()
{
	if (secstimer1 == 0)
	{
		secs1_flag = 0; //reset the attemp flag if not good
		communication_add_char_to_serial_buffer(&secs_obj.serial->TxBuffer, NAK); // sendToSecsSerialPort(Nak);
		//serialPort.Write(((char)NAK).ToString());//send the you failed sucker message 
		SecsReceivedMessageTotalErrorNum++;
		SecsReceivedMessageTimeoutErrorNum++;
		//reportError("TimeOut waiting for characters in message");
		return;         //if you found timer2 is zero we have failed
	}

	//read all available characters please,until we have recived them all, according to the length byte that was already received
	while (secs1bytesleft != 0)
	{
		//if (LastCharacterRecieved == LastCharacterRead) return;  // if (nextcharacter2 == lastcharacter2) 
		if (SecsCharWaiting() == 0) return;//no chars waiting
		uint8_t temp = GetLastChar(); //get me a character
		secs_receive_buffer[secs1receivepointer] = temp; //store the character
		secs1receivepointer++; // point to next character buffer location
		secs1charactersreceived++; //keep track of the total number of char rcvd
		secs1bytesleft--; // number of characters left to receive
		//need to add error checking for more than 255 characters
	}

	//if you get here then we have received the whole enchilada
	//including the checksum bytes so now we can do the checking
	if (!checksumreceivecheck())// not true we bombed out so cancel
	{
		checksumfailed++;
		secs1_flag = 0; //reset the attemp flag if not good
		//temp = nak;
		communication_add_char_to_serial_buffer(&secs_obj.serial->TxBuffer, NAK);
		SecsReceivedMessageTotalErrorNum++;
		SecsReceivedMessageCheckSumErrorNum++;
		//reportError("Rcv Cksum Err");
		//textBox48.Text = checksumfailed.ToString();
		return;         //if you found timer2 is zero we have failed

	}//error make appropriate replys

	//if you get here we have succesfully received a fully checksum
	//validated mesage packet of the right length. yeaaah.
	//checksum_passed++;
            
	secs1_flag = 3; //set the flag to the next value
	checksum_passed++; //let the secs2 processor know  we have secs
	communication_add_char_to_serial_buffer(&secs_obj.serial->TxBuffer, ACK); //sendToSecsSerialPort(Ack);
	//serialPort.Write(((char)ACK).ToString());//send the you passed ok                           
	return;

}


void have_secs_message()
{
	secs1_flag = 0; //turn off the secs flag,go back to idle
	//parse_rcvd_message();
	if (secs_receive_buffer[0] < 10) return;//to short, not a real message
            
	if (ParseSecsMessage(secs_receive_buffer)) {
		sprintf(ReceivedSecsCmd, "S%dF%d", CurrentSecsMessage.Stream, CurrentSecsMessage.Function);
		memcpy(&LastReceivedMessage, &CurrentSecsMessage, sizeof(SecsMsgPacket)); //save the latest received message 
		//secs_received_rows = ConvertSecsBinaryToStringList(secs_receive_buffer, secsstringReceiveList[0]); //convert the secs buffer to string list.
		secs_is_recevied = true;
		SecsMessageWaiting = 1;
		secs_rx_num++;
	}
	else
	{
		SecsReceivedMessageTotalErrorNum++;
		SecsReceivedMessageParsingErrorNum++;
	}
}

void initiate_block_transmit()
{
	communication_add_char_to_serial_buffer(&secs_obj.serial->TxBuffer, ENQ); //sendToSecsSerialPort(Enq);
	//serialPort.Write(((char)ENQ).ToString());//start the block xfer with "are you ready"  
	secstimer2 = systemconfig.secs.timerReload2 * 20; //  secstimer2default;//set the reply timeout timer 
	secs1_flag = 11; //point to the next state for the secs1 processor
	return;         //
}

/// <summary>
        /// in this state we are waiting for the eot from the guy we are trying to 
        /// talk to, if the timer2 times out we retry again, but only so many times
        /// 
        /// </summary>
void waiting_for_eot()
{
	if (secstimer2 == 0)
	{
		if (numberofretriesleft < 1)
		{
			//add failure code hereurnn 
			secs1_flag = 0; //turn off the secs flag
			secssendfail++;
			//AppendLog ? .Invoke(LOGTYPE.ERROR, LOGSESSION.SECS1, DIRECTION.NONE, "SECSII Send TIMEOUT");
			//logBox1.scrollToBottom();
			return;
		}
		numberofretriesleft--; //count down and restart the process again

		secs1_flag = 10;
		return;
	}
	//if (LastCharacterRecieved == LastCharacterRead) return;   //if (nextcharacter2 == lastcharacter2) return;
	if (SecsCharWaiting() == 0) return;//no chars waiting
	uint8_t temp2 = GetLastChar();
	if (temp2 != EOT) {
		secs1_flag = 0; //turn off the secs flag
		secssendfail++;
		SecsSendMessageTimeoutErrorNum++;
		SecsSendMessageTotalErrorNum++;
		return; //it must be a eot otherwise bye bye
	}
	//ok now you are here it is time to send that actual stream and function
	int number_of_characters_to_send = secs_transmit_buffer[0];
	//now we have the message length; but do we have the overall length
	number_of_characters_to_send = number_of_characters_to_send + 3;
	//now we have included the length byte and the 2 checsum bytes

	communication_add_buffer_to_serial_buffer(&secs_obj.serial->TxBuffer, secs_transmit_buffer, number_of_characters_to_send);
	
	secs1_flag = 12; // signal the wait for acknowledgement
}
void waiting_for_ack()
{
	if (secstimer2 == 0)
	{
		if (numberofretriesleft < 1)
		{
			//add failure code hereurnn 
			secs1_flag = 0; //turn off the secs flag
			secssendfail++;
			//SecsSendGuiDirty = true;
			return;
		}
		numberofretriesleft--; //count down and restart the process again
		secs1_flag = 10;
		return;
	}
	if (SecsCharWaiting() == 0) return;//no chars waiting
	uint8_t temp2 = GetLastChar(); //            byte temp2 = getchar2();
	if (temp2 == ACK)
	{
		secssendpass++;
		secs1_flag = 0; //don't forget to turn off the secs flag please
		return; //it must be a eot otherwise bye bye
	}

	if (numberofretriesleft < 1)
	{
		//add failure code hereurnn 
		secs1_flag = 0; //turn off the secs flag
		secssendfail++;
		SecsSendMessageTimeoutErrorNum++;
		SecsSendMessageTotalErrorNum++;
		//reportError("SECSII Send TimeOut");
		return;
	}
	numberofretriesleft--; //count down and restart the process again
	secs1_flag = 10;
	return;
}

uint16_t GetSecsCommadId(SecsMsgPacket* msg)
{
	return msg->Stream * 100 + msg->Function;
}

void ParseIncommingLineToSecsString()
{
	TestForSecsMessageReadyToSend(); //see if we should be sending any messages
	if (SecsMessageWaiting)
	{
		PrcessSecsReceivedMessage();
		SecsMessageWaiting = 0;
	}

	switch (secs1_flag)
	{
	case 0: secs1_idle(); return;
	case 1: waiting_for_length(); return;
	case 2: read_me_a_character(); return;
	case 3: have_secs_message(); return;
	case 10: initiate_block_transmit(); return;
	case 11: waiting_for_eot(); return;
	case 12: waiting_for_ack(); return;
	default: secs1_flag = 0; return;
	}
}
uint16_t SecsTimerPrescaler = 5;
void SecsTimers(void)
{
	//decrements the secs timers every 50ms
	if (SecsTimerPrescaler)
	{
		SecsTimerPrescaler--;
		return;//not time to count down yet
	}
	SecsTimerPrescaler = 5;
	if (secstimer1) secstimer1--;
	if (secstimer2) secstimer2--;
	if (secstimer3) secstimer3--;
	if (secstimer5) secstimer5--;
	if (secstimer6) secstimer6--;
	if (secstimer7) secstimer7--;
}


////////////////////////////////////////////////////////////////////////////////
uint8_t s1f1message[13] = 
{
	10,
	//length byte 3 less than total byte array
	0, 0,
	//device id template
	0x81, 1, 0x80, 1, //stream1(with reply bit set) function1, last block
	0, 0, 0, 0,
	//system bytes dummy field
	0, 0             //checksum bytes, to be filled inlater
};
        
uint8_t s1f2message[41] =
{ 
	38,//length byte 3 less than total byte array
	0, 0,//device id template
	1, 2, 0x80, 1,//stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	1, 3,//1 list 2 items
	0x41, 7,//ascii format 10 characters
	'P', 'C', 'T', 'M', '2', '6', '_',
	0x41, 7,	//ascii format 5 characters
	'S', '1', '.', 'X', 'X', 'X', '_',
	0x41, 6,	//ascii format 5 characters
	'M', '1', '.', 'X', 'X', 'X',
	0, 0
};

uint8_t s1f5message[16] = 
{ 
	13,
	//length byte 3 less than total byte array
	0, 0,
	//device id template
	0x81, 5, 0x80, 1,//stream1(with reply bit set) function1, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 1, 
	0, //Unit #
	0, 0             //checksum bytes, to be filled inlater
};

uint8_t s1f6message[26] = 
{
	23,//length byte 3 less than total byte array
	0, 0, //device id template
	1, 6, 0x80, 1,//stream1 function2, last block
	0, 0, 0, 0,//system bytes dummy field
	1, 2, //1 list  items
	0x21, 3,//ascii format 20 characters max for last wafer id
	1, 04, 0, //1, MegasonicStatus,MegasonicError,
	0xA9, 04, 0, 0,//powerHigh,powerLow,
	0, 0,//TimeHigh,TimeLow,
	0, 0
};

uint8_t s1f6VerteqSRDmessage[52] = 
{
	49,//data Lenght
	0, 0, //device id template
	1, 6, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0,//system bytes dummy field
	1, 7, //Layer 7
	0x21,9,
	//Binary 9 items <B (0x40)(0x1)(0x0)(0x0)(0x0)(0x0)(0xff)(0xff)(0x1)>
	0x40, 0x1, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0x1, 0x41, 1, //<A 0x1>   //Ascii: 1 item
	'1', 
	0x41, 1, //<A 0x0>
	'0',  
	0x41, 4,
	//<A '0000'>
	'0', '0', '0', '0',
	0x41, 4, //<A '0810'>
	'0', '8', '1', '0',
	0x41, 3, //<A '000'>
	'0', '0', '0',
	0x21, 1, //<B (0xe)>
	0xe,
	0, 0         //CheckSum bytes
};
uint8_t s2f19message[16] = {
                    0xD,            //length byte 3 less than total byte array
                    0,0,        //device id template
                    2,19,0x80,1,    //stream1 function2, last block
                    0,0,0,0,        //system bytes dummy field
                    0xA5, 01, 00,
                    0,0
};
		
uint8_t s2f20message[13] = 
{
	10,//length byte 3 less than total byte array
	0, 0, //device id template
	2, 20, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0, 0
};
uint8_t s2f20VerteqSRDmessage[16] =
{
	13, //length byte 3 less than total byte array
	0, 0,  //device id template
	2, 20, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 1, //Binary one time
	0, 0,
	0
};
uint8_t s2f21message[16] =
{
	13, //length byte 3 less than total byte array
	0, 0, //device id template
	2, 21, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 1, 0, //Command ,            //0 : Stop, 1: Start, 0A = Reset End Of Cycle
	0, 0
};
uint8_t s2f22message[16] =
{
	13, //length byte 3 less than total byte array
	0, 0, //device id template
	2, 22, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0xa5, 1, //MegasonicResponse,            //SINGLE BINARY BYTE VALUE = 0 MEANS		ALLRESET
	0, 0, 0
};
//select recipe 
uint8_t s7f1message[21] =
{ 
	18, //length byte 3 less than total byte array
	0, 0, //device id template
    7, 1, 0x80, 1, //stream1 function2, last block
    0, 0, 0, 0, //system bytes dummy field
    1, 2, //1 list 2 items
    0x20, 0x01, 0x01, //recipe number in ascii, expecting "1--10"
    0xa5, 0x01, 0x00, //second item is unsigned binary single byte
    0, 0              //checksum placeholders
};


uint8_t s7f2message[16] =
{ 
	13, //length byte 3 less than total byte array
	0, 0, //device id template
	7, 2, 0x80, 1, //stream7 function2, last block
	0, 0, 0, 0, //system bytes place holder
	0x21, 1, //MegasonicResponse,            //SINGLE BINARY BYTE 0=ok,2 = fail,3=invalid recipe
	0, 0, 0
};
uint8_t s7f3message[26] = 
{	
		0x17,            //length byte 3 less than total byte array
        0,0,        //device id template
        7,3,0x80,1,    //stream1 function2, last block
        0,0,0,0,        //system bytes dummy field
        1,2,            //1 list 2 items
        0x21, 0x03, 
        0x00, 0x00, 0x00,//3 byte
        0xa9,0x04,          //unsigned int 2 
        0x0, 0x0, 0x0, 0x0,  //Power high, Power Low, Time high, Time Low
        0, 0              //checksum placeholders
};
uint8_t s7f4message[16] =
{
	13, //length byte 3 less than total byte array
	0, 0, //device id template
	7, 4, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 1,  //MegasonicResponse,            //SINGLE BINARY BYTE VALUE = 0 MEANS ALLRESET
	0, 0, 0
};
uint8_t s7f5message[18] =
{
	0xf,            //length byte 3 less than total byte array
    0,0,        //device id template
    7,5,0x80,1,    //stream1 function2, last block
    0,0,0,0,        //system bytes dummy field
    0x1, 1, //MegasonicResponse,            //SINGLE BINARY BYTE VALUE = 0 MEANS ALLRESET
    0x41, 0x1,
    0,
    0,0
};
uint8_t s7f6message[24] = 
{
	21, //length byte 3 less than total byte array
	0, 0, //device id template
	7, 6, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	01, 02, 0x21, 1, //MegasonicResponse,            //SINGLE BINARY BYTE VALUE = 0 MEANS ALLRESET
	0, //Recipe Index 15
	0xa9, 04, 0, 0,//power
	0, 0,//time remaining
	0, 
	0
};
        
//Message Error by Unrecognized device id
uint8_t s9f1message[25] =
{
	22, //length byte 3 less than total byte array
	0, 0x80, //device id template
	9, 1, 0x80,  1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 10, //unspcified binary 10 bytes
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0             //checksum
};
//Message Error by Unrecognized stream type
uint8_t s9f3message[25] =
{
	22, //length byte 3 less than total byte array
	0, 0x80, //device id template
	9, 3, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 10, //unspcified binary 10 bytes
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0             //checksum
};
//Message Error by Unrecognized Function type
uint8_t s9f5message[25] =
{
	22, //length byte 3 less than total byte array
	0, 0x80, //device id template
	9, 5, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 10, //unspcified binary 10 bytes
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 //checksum
};
//Message Error by Unrecognized Data type
uint8_t s9f7message[25] =
{
	22, //length byte 3 less than total byte array
	0, 0x80, //device id template
	9, 7, 0x80, 1,  //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 10, //unspcified binary 10 bytes
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0//checksum
};

//Message Error by Transaction Time-out
uint8_t s9f9message[25] =
{
	22, //length byte 3 less than total byte array
	0, 0x80, //device id template
	9, 7, 0x80, 1,  //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 10, //unspcified binary 10 bytes
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0//checksum
};

//Message Error by Excessive Data size
uint8_t s9f11message[25] =
{
	22, //length byte 3 less than total byte array
	0, 0x80, //device id template
	9, 7, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 10, //unspcified binary 10 bytes
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0             //checksum
};

//Message Error by Communications Time-out
uint8_t s9f13message[25] =
{
	22, //length byte 3 less than total byte array
	0, 0x80, //device id template
	9, 7, 0x80, 1, //stream1 function2, last block
	0, 0, 0, 0, //system bytes dummy field
	0x21, 10, //unspcified binary 10 bytes
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0             //checksum
};


uint8_t SystemByte1 = 0;
uint8_t SystemByte2 = 0;
uint8_t SystemByte3 = 0;
uint8_t SystemByte4 = 0;

void systemidme(uint8_t* secssendbuffer)
{
	secssendbuffer[2] = SECS_DEVICE_ID;
	secssendbuffer[1] = 0x8a; // 0;       //now the device id is plugged in
	systembytes++;
	if (systembytes == 0) systembyte34++;

	uint8_t tempbyte1 = (uint8_t)(systembyte34 / 256); //convert to float for math
	secssendbuffer[7] = tempbyte1;
	tempbyte1 = (uint8_t)(systembyte34 % 256);
	secssendbuffer[8] = tempbyte1;
	tempbyte1 = (uint8_t)(systembytes / 256); //convert to float for math
	secssendbuffer[9] = tempbyte1;
	tempbyte1 = (uint8_t)(systembytes % 256);
	secssendbuffer[10] = tempbyte1;
}
void systemidyou(uint8_t* secssendbuffer)
{
	//	if (settings.DeviceID > 255) settings.DeviceID = 1; //not allowed >255
	
	secssendbuffer[2] = SECS_DEVICE_ID; //now the device id is plugged in
	secssendbuffer[1] = 0x80; //if (WorkingSecsData.SerialType == 0) secssendbuffer[1] = 0x80;
	systembytes++;
	secssendbuffer[7] = SystemByte1;
	secssendbuffer[8] = SystemByte2;
	secssendbuffer[9] = SystemByte3;
	secssendbuffer[10] = SystemByte4;
}

void checksummer(uint8_t* secssendbuffer)
{
	int messagelength = secssendbuffer[0];

	int cksum = 0; //zero out my checksum variable
	int count = 1;

	for (count = 1; count < (messagelength + 1); count++)
	{
		cksum += secssendbuffer[count]; //add up the next character
	}
	secssendbuffer[messagelength + 2] = (uint8_t)(cksum & 0xff); 
	secssendbuffer[messagelength + 1] = (uint8_t)((cksum & 0xff00) / 256);
}

void VerteqMeg_idme(uint8_t* secssendbuffer)
{	
	secssendbuffer[2] = SECS_DEVICE_ID;
	secssendbuffer[1] = 0x8a; // 0;       //now the device id is plugged in
}
void VerteqSRD_idme(uint8_t* secssendbuffer)
{
	secssendbuffer[2] = 0x28;
	secssendbuffer[1] = 0x0a; // 0;       //now the device id is plugged in
}
void systemMe(uint8_t* secssendbuffer)
{
	//
	systembytes++;
	if (systembytes == 0) systembyte34++;
	uint8_t tempbyte1 = (uint8_t)(systembyte34 / 256); //convert to float for math
	secssendbuffer[7] = tempbyte1;
	tempbyte1 = (uint8_t)(systembyte34 % 256);
	secssendbuffer[8] = tempbyte1;
	tempbyte1 = (uint8_t)(systembytes / 256); //convert to float for math
	secssendbuffer[9] = tempbyte1;
	tempbyte1 = (uint8_t)(systembytes % 256);
	secssendbuffer[10] = tempbyte1;
}

void send_secs_message(uint8_t* message, uint16_t size)
{
	if (size < 1) return;//if no length then do not schedule it for tranmission.
	SecsSendQue[secsSendQuePointerHead][0] = size; //(byte)message.Length; //store the length of the message please
	SecsSendQue[secsSendQuePointerHead][1] = 0; //store the stype for future processing between secs message and stype controls such as linq request
	for (int count = 0; count < size; count++)
	{
		SecsSendQue[secsSendQuePointerHead][count + 2] = message[count];
	}
	secsSendQuePointerHead++; //move pointer to next que in the array for the next message
	secsSendQuePointerHead &= 7; //only 8 entrys before spinning in the circular que
}

void SendSecsCommand(uint8_t* secsbuf, uint16_t size)
{
	if (systemconfig.serial2.mode != SERIAL2_MODE_SECS) return; //1: secs mode, 0: simple mode
	systemidme(secsbuf); //plug in the necessary id  info
	checksummer(secsbuf); //plug in the final checsum
	send_secs_message(secsbuf, size);
}

void PrcessSecsReceivedMessage()
{
	SystemByte1 = LastReceivedMessage.SystemByte1;
	SystemByte1 = LastReceivedMessage.SystemByte2;
	SystemByte1 = LastReceivedMessage.SystemByte1;
	uint16_t secsId = GetSecsCommadId(&LastReceivedMessage); //use id as Stream * 100 + function
	uint8_t channel, res;
	switch (secsId)
	{
	case S1F1:
		memcpy(s1f2message + 25, MajorStep, 5); //SC01 Major version 
		memcpy(s1f2message + 34, amplifier_major_step, 5); //Meg407 Major version
		SendSecsCommand(s1f2message, sizeof(s1f2message));
		break;
	case S1F5:
		{
			channel  = (uint8_t)LastReceivedMessage.RawData[13];
			if (channel < MAX_CHANNELS)
			{
				sprintf(temp_string, "M805 T%d\n", channel);
				communication_add_string_to_serial_buffer(&MasterCommPort->TxBuffer, temp_string);
				//when we received P*, it send this.
//				s1f6message[15] = channel; // channel
//				s1f6message[16] = (uint8_t)amplifier_processmode; // Status of Meg407
//				s1f6message[17] = (uint8_t)0; // Error of Meg407
//				s1f6message[20] = (uint8_t)(amplifier.RF_Channels[channel].vars.ForwordPower >> 8) & 0x00ff; // high bit of Power
//				s1f6message[21] = (uint8_t)(amplifier.RF_Channels[channel].vars.ForwordPower & 0x00ff); // high bit of Power
//				s1f6message[22] = (uint8_t)(amplifier.RF_Channels[channel].vars.ActualTimeSec >> 8) & 0x00ff; // high bit of Time
//				s1f6message[23] = (uint8_t)(amplifier.RF_Channels[channel].vars.ActualTimeSec & 0x00ff); // high bit of Time	
			}
			else
			{
				s1f6message[15] = channel; // channel
				s1f6message[16] = 0; // Status of Meg407
				s1f6message[17] = 128; // Error of Meg407 
				s1f6message[20] = 0; // high bit of Power
				s1f6message[21] = 0; // high bit of Power
				s1f6message[22] = 0; // high bit of Time
				s1f6message[23] = 0;
				SendSecsCommand(s1f6message, sizeof(s1f6message));
			}
			break;
		}
	case S2F19:
		SendSecsCommand(s2f20message, sizeof(s2f20message));
		break;
	case S2F21: 
		{
			res = 0; // ACKNOWLEDGED
			switch (LastReceivedMessage.RawData[13])
			{
			case 0: // STOP 
				break;
			case 0x1B: //Cancel Process
				communication_add_string_to_serial_buffer(&MasterCommPort->TxBuffer, "M801 I661 T255\n"); 
				break;
			case 1: //START PROCESS
				communication_add_string_to_serial_buffer(&MasterCommPort->TxBuffer, "M801 I660 T255\n"); 
				break;
			case 0xA: //RESET END OF CYCLE
				break;
			default:
				res = 1; // INVALID COMMAND
				//res = 2; // UNABLE TO EXECUTE COMMAND
				break;
			}
			s2f22message[13] = res; // response
			SendSecsCommand(s2f22message, sizeof(s2f22message));
			break;//remote start
		}
	case S7F1: // Select Unit
		channel = (uint8_t)LastReceivedMessage.RawData[15];
		if (channel < MAX_CHANNELS) res = 0;
		else res = 03; //INVALID UNIT #
		s7f2message[13] = res;
		SendSecsCommand(s7f2message, sizeof(s7f2message));
		break;
	case S7F3:  //Download Recipe
		channel = (uint8_t)LastReceivedMessage.RawData[15];
		if (channel >= MAX_CHANNELS) res = 04; // INVALID UNIT #
		else
		{
			amplifier.RF_Channels[channel].settings.ProgPower1 = (LastReceivedMessage.RawData[20] << 8) + LastReceivedMessage.RawData[21];
			amplifier.RF_Channels[channel].settings.DesiredTimeSec = (LastReceivedMessage.RawData[22] << 8) + LastReceivedMessage.RawData[23];
			sprintf(temp_string, "M801 B%d I0\n", amplifier.RF_Channels[channel].settings.ProgPower1);
			//sprintf(temp_string, "M801 B%d I0\n", amplifier.RF_Channels[channel].settings.DesiredTimeSec);
		}
		//bool Result = DownloadRecipe(values);
		SendSecsCommand(s7f4message, sizeof(s7f4message));
		break;
	case S7F5:  //Upload Recipe		
		channel = (uint8_t)LastReceivedMessage.RawData[15];
		s7f6message[15] = channel;
		s7f6message[20] = channel >= MAX_CHANNELS ? 0 : (uint8_t)(amplifier.RF_Channels[channel].settings.ProgPower1 >> 8) & 0x00ff; // high bit of Power
		s7f6message[21] = channel >= MAX_CHANNELS ? 0 : (uint8_t)(amplifier.RF_Channels[channel].settings.ProgPower1 & 0x00ff); // high bit of Power
		s7f6message[22] = channel >= MAX_CHANNELS ? 0 : (uint8_t)(amplifier.RF_Channels[channel].settings.DesiredTimeSec  >> 8) & 0x00ff; // high bit of Time
		s7f6message[23] = channel >= MAX_CHANNELS ? 0 : (uint8_t)(amplifier.RF_Channels[channel].settings.DesiredTimeSec & 0x00ff); // high bit of Time
		SendSecsCommand(s7f6message, sizeof(s7f6message));
		break;
	default:
		SendSecsCommand(s9f7message, sizeof(s9f7message));
		break;
	}
}