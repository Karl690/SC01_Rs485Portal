#include "string.h"
#include "M_Codes.h"
#include "cmdprocessor.h"
#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"

void M_Code_M640()
{
	if (!strlen(ExecutionPtr->Comment)) return;
	communication_add_string_to_serial_buffer(&ComUart2.TxBuffer, ExecutionPtr->Comment);
}
