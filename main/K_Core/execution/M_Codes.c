#include "string.h"
#include "M_Codes.h"
#include "cmdprocessor.h"
#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"
#include "K_Core/supply/supply.h"

void M_Code_M640()
{
	if (ARG_I_PRESENT)
	{
		if (ARG_I == 0) supply_turn_off_voltage();
		else if (ARG_I == 1) supply_turn_on_voltage();
		return;
	}
	else if (ARG_V_PRESENT && ARG_C_PRESENT)
	{
		supply_set_teslaman_voltage_current(ARG_V, ARG_C);
		return;
	}
	else if (ARG_R_PRESENT)
	{
		supply_read_teslaman_status();
		return;
	}
	else if (strlen(ExecutionPtr->Comment))
	{
		communication_add_string_to_serial_buffer(&ComUart2.TxBuffer, ExecutionPtr->Comment);	
	}
}

void M_Code_M641()
{
	if (ARG_S_MISSING) return;
	if (ARG_S == 0) supply_computer_control_off();
	else if (ARG_S == 1) supply_computer_control_on();
}

void M_Code_M642()
{
	if (ARG_S_MISSING) return;
	if (ARG_S == 0) supply_turn_off_set_voltage();
	else if (ARG_S == 1) supply_turn_on_set_voltage();
}