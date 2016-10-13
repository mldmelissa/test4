/** @file serial_circular_buffer_service.cpp
 *  @brief Serial circular buffer service
 *  
 *  This module contains the implementation of the function prototypes as 
 *  defined in the header file.
 *  
 *  @author Adam Porsch
 *  @bug No known bugs.
 */

#include <string.h>
#include "serial_circular_buffer_service.h"

//these two pointers are used by the microprocessor specific UART IRQ handlers
serial_circular_buffer *UART0_ISR_instance_ptr = NULL;
serial_circular_buffer *UART1_ISR_instance_ptr = NULL;

#pragma region Public Class Member Functions
void serial_circular_buffer::init(uart_t uart_port_base_addr,
								  char *Rx_buffer_ptr,
								  uint32_t Rx_buffer_size_in_bytes,
								  char *Tx_buffer_ptr,
								  uint32_t Tx_buffer_size_in_bytes,
								  uint32_t baud_rate,
								  uart_parity_selection_t parity)
{	
	this->uart_peripheral_base_address = uart_port_base_addr;
	
	if(uart_port_base_addr == UART_PORT_0)
	{
		this->pdc_peripheral_base_address = PDC_UART_PORT_0;
		UART0_ISR_instance_ptr = this;		
	}
	else if(uart_port_base_addr == UART_PORT_1)
	{
		this->pdc_peripheral_base_address = PDC_UART_PORT_1;
		UART1_ISR_instance_ptr = this;
	}
	
	HAL_UART_INITIAILZE(this->uart_peripheral_base_address, baud_rate, (uint32_t)parity);
	
	this->rx_buffer_size = Rx_buffer_size_in_bytes;
	this->rx_buffer = Rx_buffer_ptr;
	this->tx_buffer_size = Tx_buffer_size_in_bytes;
	this->pdc_tx_buffer = Tx_buffer_ptr;
	
	this->rx_buffer_tail_index = 0;
	this->tx_buffer_head_index = 0;
	this->tx_buffer_tail_index = 0;
	this->pdc_Tx_in_progress = false;
	
	HAL_PDC_RX_INIT_NO_NEXT(this->pdc_peripheral_base_address, (uint32_t)this->rx_buffer, this->rx_buffer_size);
	
	HAL_PDC_ENABLE_TRANSMITTER_TRANSFER();
	HAL_PDC_ENABLE_RECEIVER_TRANSFER();
	
	HAL_UART_ENABLE_RX_BUFFER_FULL_INTERRUPT();
	HAL_UART_DISABLE_TX_BUFFER_EMPTY_INTERRUPT();
	
	
	if(uart_port_base_addr == UART_PORT_0)
	{
		ENABLE_IRQ(UART0_IRQ_NUM);
	}
	else if(uart_port_base_addr == UART_PORT_1)
	{
		ENABLE_IRQ(UART1_IRQ_NUM);
	}

	
}

char serial_circular_buffer::get_latest_byte()
{
	char return_byte;
	
	return_byte = this->rx_buffer[this->rx_buffer_tail_index];
	this->increment_rx_buffer_tail_index(1);
	
	return(return_byte);
}

uint32_t serial_circular_buffer::get_number_of_unread_bytes(void)
{
	int32_t difference = 0;
	
	difference = this->get_rx_buffer_head_index() - this->rx_buffer_tail_index;
	
	//the following conditional check handles the scenario where the head index has rolled back over to beginning of buffer
	if(difference < 0)
	{
		difference += this->rx_buffer_size;						
	}
	
	return((uint32_t)difference);
}

void serial_circular_buffer::copy_packet_into_Tx_buffer_and_transmit(char* serialized_data_to_transmit, uint32_t number_of_bytes_to_transmit)
{
	uint32_t first_contiguous_block_size = 0;
	uint32_t second_contiguous_block_size = 0;
	uint32_t initial_tx_buffer_head_index = 0;
	uint8_t circular_buffer_rollover_condition = 0;
	
	initial_tx_buffer_head_index = this->tx_buffer_head_index;	//save off the original circular buffer head index value for later use in this function
	
	//first, determine if the packet we're transmitting needs to be divided up between the end and the beginning of the circular buffer and handle it
	if((number_of_bytes_to_transmit + this->tx_buffer_head_index) > this->tx_buffer_size)
	{
		circular_buffer_rollover_condition = 1;
		first_contiguous_block_size = this->tx_buffer_size - this->tx_buffer_head_index;
		second_contiguous_block_size = (number_of_bytes_to_transmit + this->tx_buffer_head_index) - this->tx_buffer_size;		
	}
	else
	{
        first_contiguous_block_size = number_of_bytes_to_transmit;
	}

	//copy the first contiguous block of packet bytes from the circular buffer to the PDC Tx buffer
	memcpy(&(this->pdc_tx_buffer[this->tx_buffer_head_index]), serialized_data_to_transmit, first_contiguous_block_size);
	this->increment_tx_buffer_head_index(first_contiguous_block_size);
	
	//if applicable, copy the remaining packet bytes to the beginning of the circular buffer
	if(circular_buffer_rollover_condition)
	{
		memcpy(&(this->pdc_tx_buffer[this->tx_buffer_head_index]), &(serialized_data_to_transmit[first_contiguous_block_size]), second_contiguous_block_size);
		this->increment_tx_buffer_head_index(second_contiguous_block_size);
	}

	//only initiate a new transmit if the PDC not currently transmitting any data. This allows multiple application threads to queue up outgoing data in the buffer
	if(this->pdc_Tx_in_progress == false)
	{
		this->pdc_Tx_in_progress = true;
		
		//"pre-load" tail so when ISR fires, it will see we've already transmitted the "first_block_size" amount of bytes
		this->increment_tx_buffer_tail_index(first_contiguous_block_size);			
		this->initiate_PDC_Tx(&(this->pdc_tx_buffer[initial_tx_buffer_head_index]), first_contiguous_block_size);
		
		/*if we've determined earlier in this function that a roll over condition exists, the PDC tx ISR will see bytes are still sitting in the circular 
		 buffer needing transmitted and call initiate_PDC_Tx() again to send those bytes out */
	}
	
}
#pragma endregion Public Class Member Functions

#pragma region Private Class Member Functions
uint32_t serial_circular_buffer::get_rx_buffer_head_index(void)
{
	return(this->rx_buffer_size - HAL_PDC_READ_RECEIVE_COUNTER_VALUE());
}

void serial_circular_buffer::increment_rx_buffer_tail_index(uint32_t increment_index)
{
	this->rx_buffer_tail_index = (this->rx_buffer_tail_index + increment_index) % this->rx_buffer_size;
}

void serial_circular_buffer::increment_tx_buffer_head_index(uint32_t increment_index)
{
	this->tx_buffer_head_index = (this->tx_buffer_head_index + increment_index) % this->tx_buffer_size;
}

void serial_circular_buffer::increment_tx_buffer_tail_index( uint32_t increment_index)
{
	this->tx_buffer_tail_index = (this->tx_buffer_tail_index + increment_index) % this->tx_buffer_size;
}

uint32_t serial_circular_buffer::get_number_of_unsent_bytes()
{
	int32_t difference = 0;
	
	difference = this->tx_buffer_head_index - this->tx_buffer_tail_index;
	
	//the following conditional check handles the scenario where the head index has rolled back over to beginning of buffer	
	if(difference < 0)
	{
		difference += this->tx_buffer_size;
	}
	
	return((uint32_t)difference);
}

void serial_circular_buffer::initiate_PDC_Tx(char *pointer_to_Tx_buffer, uint32_t bytes_to_transfer)
{
	HAL_PDC_TX_INIT_NO_NEXT(this->pdc_peripheral_base_address, (uint32_t)(pointer_to_Tx_buffer), bytes_to_transfer);	
	HAL_UART_ENABLE_TX_BUFFER_EMPTY_INTERRUPT();

}
#pragma endregion Private Class Member Functions

#pragma region UART ISR Handlers
void UART0_Handler(void)
{
	UART0_ISR_instance_ptr->serial_circular_buffer_irq_handler();
}

void UART1_Handler(void)
{
	UART1_ISR_instance_ptr->serial_circular_buffer_irq_handler();
}

void serial_circular_buffer::serial_circular_buffer_irq_handler(void)
{
	uint32_t	number_of_unsent_tx_bytes;
	uint32_t	number_of_bytes_to_send;

	if(HAL_UART_IS_RECEIVE_BUFFER_FULL())
	{
		//if here, the rx circular buffer needs to roll over. Re-initialize the PDC with the address of the first element of the Rx circular buffer
		HAL_PDC_RX_INIT_NO_NEXT(this->pdc_peripheral_base_address, (uint32_t)this->rx_buffer, this->rx_buffer_size);
	}

	if(HAL_UART_IS_TRANSMIT_BUFFER_EMPTY())
	{
		number_of_unsent_tx_bytes = this->get_number_of_unsent_bytes();

		if(number_of_unsent_tx_bytes)
		{
			if(this->tx_buffer_head_index < this->tx_buffer_tail_index)						//check for rollover (i.e. bytes to send at the end of the buffer, and the beginning)
			{
				number_of_bytes_to_send = this->tx_buffer_size - tx_buffer_tail_index;		//if packet is split up between end and beginning of buffer,
			}																				//send contiguous end of buffer 1st. ISR will then fire again, to send remainder at beginning of buffer.
			else
			{
				number_of_bytes_to_send = number_of_unsent_tx_bytes;
			}
			this->initiate_PDC_Tx(&(this->pdc_tx_buffer[this->tx_buffer_tail_index]), number_of_bytes_to_send);
			this->increment_tx_buffer_tail_index(number_of_bytes_to_send);			
			this->pdc_Tx_in_progress = true;
		}
		else
		{		
			this->pdc_Tx_in_progress = false;
			HAL_UART_DISABLE_TX_BUFFER_EMPTY_INTERRUPT();
		}
	}
}
#pragma endregion UART ISR Handlers