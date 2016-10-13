/** @file HAL_serial_circular_buffer.cpp
 *  @brief Serial circular buffer Hardware Abstraction Layer (HAL)
 *  
 *  This module contains the implementation of the function prototypes as 
 *  defined in the header file.
 *  
 *  @author Adam Porsch
 *  @bug No known bugs.
 */

#include "HAL_serial_circular_buffer.h"

void HAL_UART_INITIAILZE(uart_t uart_peripheral_base_address, uint32_t baudrate, uint32_t parity)
{
	uint32_t parity_reg_value = 0;
	
	//Reset and disable receiver & transmitter
	uart_peripheral_base_address->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS | UART_CR_TXDIS;
	
	// Connect port pins for this instance
	if (uart_peripheral_base_address == UART0) 
	{
		PIOA->PIO_PDR = PIO_PDR_P9;                     // Enable RX pin to function as peripheral
		PIOA->PIO_ABCDSR[0] &= ~PIO_ABCDSR_P9;          // Connect peripheral to pin (URXD0 is peripheral A for pin 9 on port A)
		PIOA->PIO_ABCDSR[1] &= ~PIO_ABCDSR_P9;
		PIOA->PIO_PDR = PIO_PDR_P10;                    // Enable TX pin to function as peripheral
		PIOA->PIO_ABCDSR[0] &= ~PIO_ABCDSR_P10;         // Connect peripheral to pin (UTXD0 is peripheral A for pin 10 on port A)
		PIOA->PIO_ABCDSR[1] &= ~PIO_ABCDSR_P10;
	}
	else if (uart_peripheral_base_address == UART1) 
	{
		PIOA->PIO_PDR = PIO_PDR_P5;                     // Enable RX pin to function as peripheral
		PIOA->PIO_ABCDSR[0] &= ~PIO_ABCDSR_P5;          // Connect peripheral to pin (URXD1 is peripheral C for pin 5 on port A)
		PIOA->PIO_ABCDSR[1] |= PIO_ABCDSR_P5;
		PIOA->PIO_PDR = PIO_PDR_P6;                     // Enable TX pin to function as peripheral
		PIOA->PIO_ABCDSR[0] &= ~PIO_ABCDSR_P6;          // Connect peripheral to pin (UTXD1 is peripheral C for pin 6 on port A)
		PIOA->PIO_ABCDSR[1] |= PIO_ABCDSR_P6;
	}
	
	//Configure baud rate
	uart_peripheral_base_address->UART_BRGR = UART_BRGR_CD((uint32_t)(SystemCoreClock/((baudrate)*16)));
	
	//Configure parity (integer enum parity value passed in matches bit definitions)
	parity_reg_value = (parity << UART_MR_PAR_Pos) & UART_MR_PAR_Msk;
	uart_peripheral_base_address->UART_MR = parity_reg_value;
	
	//disable PDC since it's not initialized yet
	uart_peripheral_base_address->UART_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;
	
	//Enable receiver and transmitter
	uart_peripheral_base_address->UART_CR = UART_CR_RXEN | UART_CR_TXEN;
	
}

void HAL_PDC_RX_INIT_NO_NEXT(pdc_t pdc_peripheral_base_address, uint32_t address, uint32_t size)
{
	pdc_peripheral_base_address->PERIPH_RPR = address;
	
	//writing to the RCR register kicks off the PDC, therefore it must be written after the address
	pdc_peripheral_base_address->PERIPH_RCR = size;
	
}

void HAL_PDC_TX_INIT_NO_NEXT(pdc_t pdc_peripheral_base_address, uint32_t address, uint32_t size)
{
	pdc_peripheral_base_address->PERIPH_TPR = address;
	
	//writing to the RCR register kicks off the PDC, therefore it must be written after the address
	pdc_peripheral_base_address->PERIPH_TCR = size;
	
}