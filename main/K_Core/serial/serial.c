#include "configure.h"
#include "main.h"

#include "serial.h"
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include "L_Core/bluetooth/ble.h"
#include "K_Core/communication/communication.h"
#include "L_Core/ui/ui-comm.h"
#include "K_Core/simple/simple.h"
#include "K_Core/secs/secs.h"
int serial_uart_fd = -1;


uint8_t serial_uart1_rx_buffer[RX_BUF_SIZE];
uint8_t serial_uart1_rx_urgent_buffer[RX_BUF_SIZE];
uint8_t serial_uart1_tx_buffer[TX_BUF_SIZE];

uint8_t serial_uart2_rx_buffer[RX_BUF_SIZE];
uint8_t serial_uart2_rx_urgent_buffer[RX_BUF_SIZE];
uint8_t serial_uart2_tx_buffer[TX_BUF_SIZE];

uint8_t serial_rx_work_buffer[RX_BUF_SIZE];
uint8_t serial_rx_work_urgent_buffer[RX_URGENT_BUF_SIZE];
COMPORT ComUart1, ComUart2;
COMPORT* MasterComPort = &ComUart1;
uint8_t serial_uart1_last_read_buffer[256];
uint8_t serial_uart2_last_read_buffer[256];

bool serial_valid_pins(uint8_t pin)
{
	if ((pin >= 10 && pin <= 14) || pin == 21 || pin == SERIAL_485_TXD_PIN || pin == SERIAL_485_RXD_PIN) return true;
	return false;
}
void serial_uart_init(uint8_t port, int tx_pin, int rx_pin, int baud, int rts_pin, int cts_pin, bool is485)
{
	if (!serial_valid_pins(tx_pin)) return;
	if (!serial_valid_pins(rx_pin)) return;
	
	const uart_config_t uart_config = {
		.baud_rate = baud,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_APB,
	};
	// We won't use a buffer for sending data.
	uart_driver_install(port, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
	uart_param_config(port, &uart_config);
	uart_set_pin(port, tx_pin, rx_pin, rts_pin, cts_pin);
	if (is485)
	{
		// Set RS485 half duplex mode	
		ESP_ERROR_CHECK(uart_set_mode(port, UART_MODE_RS485_HALF_DUPLEX));
	}
	else
	{
		// Enable UART DMA
		uart_set_mode(port, UART_MODE_UART);	
	}
	
}


void serial_uart_update_config(uint8_t port, int tx_pin, int rx_pin, int baud)
{
	const uart_config_t uart_config = {
		.baud_rate = baud,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		//.source_clk = UART_SCLK_APB,
	};
	uart_param_config(port, &uart_config);
	uart_set_pin(port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}
bool serial_uart_write_byte(COMPORT* comport, char byte)
{
	if (uart_write_bytes(comport->uart_id, &byte, 1) != 1) {
		ESP_LOGE(TAG, "Send data critical failure.");
		// add your code to handle sending failure here
		return false;
	}
	comport->TxIndicator = 3;
	comport->NumberOfCharactersSent++;
	return true;
}

void serial_uart1_read_task(void* param)
{
	uint8_t buffer[RX_BUF_SIZE];
	while (1)
	{
		if (!IsInitialized) {
			vTaskDelay(10);
			continue;
		}
		int len = uart_read_bytes(UART_NUM_1, buffer, RX_BUF_SIZE, 10 / portTICK_PERIOD_MS); //(100 / portTICK_PERIOD_MS));	
		if (len > 0) {
			ComUart1.NumberOfCharactersReceived += len;
			ComUart1.RxIndicator = INDICATER_COUNTDOWN;
			communication_add_buffer_to_serial_buffer(&ComUart1.RxBuffer, buffer, len);
		}
	}
}
void serial_uart2_read_task(void* param)
{
	uint8_t buffer[RX_BUF_SIZE /2]; // 2kbytes
	while (1)
	{
		if (!IsInitialized) {
			vTaskDelay(10);
			continue;
		}
		int len = uart_read_bytes(UART_NUM_2, buffer, RX_BUF_SIZE /2, 10 / portTICK_PERIOD_MS); //(10 / portTICK_PERIOD_MS)
		if (len > 0)
		{
			ComUart2.NumberOfCharactersReceived += len;
			ComUart2.RxIndicator = INDICATER_COUNTDOWN;
			communication_add_buffer_to_serial_buffer(&ComUart2.RxBuffer, buffer, len);	
		}
	}
}



void serial_init()
{
	// initialize uart devices
	serial_uart_init(UART_NUM_1, systemconfig.serial1.tx_pin, systemconfig.serial1.rx_pin, systemconfig.serial1.baud, SERIAL_UART1_RTS_PIN, SERIAL_UART1_CTS_PIN, false);
	
	if (systemconfig.serial1.tx_pin != systemconfig.serial2.tx_pin 
		&& systemconfig.serial1.rx_pin != systemconfig.serial2.rx_pin)
	{
		if (systemconfig.serial2.is_485)
		{
			
			serial_uart_init(UART_NUM_2, SERIAL_485_TXD_PIN, SERIAL_485_RXD_PIN, 9600, SERIAL_485_RTS_PIN, SERIAL_UART2_CTS_PIN, true);		
		}
		else
		{
			serial_uart_init(UART_NUM_2, systemconfig.serial2.tx_pin, systemconfig.serial2.rx_pin, systemconfig.serial2.baud, SERIAL_UART2_RTS_PIN, SERIAL_UART2_CTS_PIN, false);		
		}
		
	}
	
	// initialize buffers
	communication_buffers_serial_init(UART_NUM_1, &ComUart1, serial_uart1_rx_buffer, serial_uart1_rx_urgent_buffer, serial_uart1_tx_buffer);
	communication_buffers_serial_init(UART_NUM_2, &ComUart2, serial_uart2_rx_buffer, serial_uart2_rx_urgent_buffer, serial_uart2_tx_buffer);
	
	RawRxComBuffer.buffer     			= serial_rx_work_buffer;
	RawRxComBuffer.Buffer_Size			= RX_BUF_SIZE; //this number is used to keep in bounds
	RawRxComBuffer.Head					= 0; //start of que
	RawRxComBuffer.Tail					= 0; //end of the que
	RawRxComBuffer.RxLineCount			= 0; // if there is a valid command waiting
	RawRxComBuffer.commandPtr =			RawRxComBuffer.command;
	memset(RawRxComBuffer.buffer, 0, RawRxComBuffer.Buffer_Size);
	//	//urgent rx buffer
	RawRxUrgentComBuffer.buffer     			= serial_rx_work_urgent_buffer;
	RawRxUrgentComBuffer.Buffer_Size			= RX_BUF_SIZE; //this number is used to keep in bounds
	RawRxUrgentComBuffer.Head					= 0; //start of que
	RawRxUrgentComBuffer.Tail					= 0; //end of the que
	RawRxUrgentComBuffer.RxLineCount			= 0; // if there is a valid command waiting
	RawRxUrgentComBuffer.commandPtr =			RawRxUrgentComBuffer.command;
	memset(RawRxUrgentComBuffer.buffer, 0, RawRxUrgentComBuffer.Buffer_Size);
	
	xTaskCreatePinnedToCore(serial_uart1_read_task, "uart_thread1", RX_BUF_SIZE * 4, NULL, 10, NULL, 1);
	// xTaskCreate(serial_uart2_read_task, "uart_thread2", RX_BUF_SIZE * 4, NULL, configMAX_PRIORITIES - 2, NULL); //configMAX_PRIORITIES - 1,
	xTaskCreatePinnedToCore(serial_uart2_read_task, "uart_thread2", RX_BUF_SIZE * 4, NULL, 10, NULL, 1);
}
