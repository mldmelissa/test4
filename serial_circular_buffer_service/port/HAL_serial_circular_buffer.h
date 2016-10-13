/** @file HAL_serial_circular_buffer.h
 *  @brief Serial circular buffer Hardware Abstraction Layer (HAL)
 *  
 *  This module contains the macros and function prototypes needed to interface
 *  the serial circular buffer service to the microprocessor specific  
 *  UART peripheral and Peripheral DMA Controller (PDC) memory mapped registers.
 *  
 *  @author Adam Porsch
 *  @bug No known bugs.
 */


#ifndef HAL_SERIAL_CIRCULAR_BUFFER_H_
#define HAL_SERIAL_CIRCULAR_BUFFER_H_

#include "sam.h"


#define UART_PORT_0				(UART0)
#define UART_PORT_1				(UART1)
#define UART0_IRQ_NUM			(UART0_IRQn)
#define UART1_IRQ_NUM			(UART1_IRQn)
#define PDC_UART_PORT_0			(PDC_UART0)
#define PDC_UART_PORT_1			(PDC_UART1)
#define IRQ_type				(IRQn_Type)
#define ENABLE_IRQ(IRQ_num)		NVIC_EnableIRQ(IRQ_num)
typedef Uart* uart_t;
typedef Pdc*  pdc_t;

extern uint32_t SystemCoreClock;

/**
 * @brief Initializes the UART used by the serial circular buffer service
 * 
 * @param uart_peripheral_base_address base memory address for microprocessor UART peripheral
 * @param baudrate UART baud rate, in base units of bits/second
 * @param parity integer value used by microprocessor UART register to configure parity as defined by uart_parity_selection_t
 * 
 * @return void
 */
void HAL_UART_INITIAILZE(uart_t uart_peripheral_base_address, uint32_t baudrate, uint32_t parity);
#define HAL_UART_ENABLE_TX_BUFFER_EMPTY_INTERRUPT()		(this->uart_peripheral_base_address->UART_IER = UART_IER_TXBUFE)
#define HAL_UART_DISABLE_TX_BUFFER_EMPTY_INTERRUPT()	(this->uart_peripheral_base_address->UART_IDR = UART_IDR_TXBUFE)
#define HAL_UART_ENABLE_RX_BUFFER_FULL_INTERRUPT()		(this->uart_peripheral_base_address->UART_IER = UART_IER_RXBUFF)
#define HAL_UART_DISABLE_RX_BUFFER_FULL_INTERRUPT()		(this->uart_peripheral_base_address->UART_IDR = UART_IDR_RXBUFF)
#define HAL_UART_IS_RECEIVE_BUFFER_FULL()				(this->uart_peripheral_base_address->UART_SR & UART_SR_RXBUFF)
#define HAL_UART_IS_TRANSMIT_BUFFER_EMPTY()				(this->uart_peripheral_base_address->UART_SR & UART_SR_TXBUFE)
#define HAL_UART_SET_BUAD(rate)							(this->uart_peripheral_base_address->UART_BRGR = UART_BRGR_CD((uint32_t)(SystemCoreClock/((rate)*16))))



/**
 * @brief Initializes the UART Rx PDC module
 * 
 * This function initializes the Rx PDC with the number of bytes to be received and the memory address
 * of where those bytes will be received. In the context of the serial circular buffer service, this
 * function is called periodically from the serial_circular_buffer_irq_handler when the pre-defined number of 
 * bytes have arrived and the peripheral needs reinitialized to the original buffer starting address and
 * buffer size, creating "circular buffer" effect.
 * 
 * The "NO NEXT" designation is used to let the developer know that these functions will not accept an address and 
 * size for the next PDC transfer, despite the fact the PDC peripheral has the capability for this.
 * 
 * @param pdc_peripheral_base_address base memory address for the microprocessor UART specific PDC peripheral
 * @param address address to the buffer in memory where the PDC will automatically transfer incoming bytes
 * @param size the size of the reception buffer, in bytes
 * 
 * @return void
 */
void HAL_PDC_RX_INIT_NO_NEXT(pdc_t pdc_peripheral_base_address, uint32_t address, uint32_t size);


/**
 * @brief Initializes the UART Tx PDC module
 * 
 * This function initializes the Tx PDC with the number of bytes to be transferred and the memory address
 * of where the outgoing bytes are located. In the context of the serial circular buffer service, this
 * function is called whenever the serial circular buffer service needs to transmit a packet that's queued up
 * in the circular buffer. It's also called from serial_circular_buffer_irq_handler when a "rollover" condition 
 * occurs and there are still bytes to be transmitted in the circular buffer once the first set of contiguous bytes
 * in memory have already been transfered.
 *
 * The "NO NEXT" designation is used to let the developer know that these functions will not accept an address and
 * size for the next PDC transfer, despite the fact the PDC peripheral has the capability for this.
 * 
 * @param pdc_peripheral_base_address base memory address for the microprocessor UART specific PDC peripheral
 * @param address address to the buffer in memory where the PDC will automatically retrieve outgoing bytes
 * @param size the size of the transmission buffer, in bytes
 * 
 * @return void
 */
void HAL_PDC_TX_INIT_NO_NEXT(pdc_t pdc_peripheral_base_address, uint32_t address, uint32_t size);

#define HAL_PDC_ENABLE_TRANSMITTER_TRANSFER()			(this->pdc_peripheral_base_address->PERIPH_PTCR = PERIPH_PTCR_TXTEN)
#define HAL_PDC_ENABLE_RECEIVER_TRANSFER()				(this->pdc_peripheral_base_address->PERIPH_PTCR = PERIPH_PTCR_RXTEN)
#define HAL_PDC_DISABLE_TRANSMITTER_TRANSFER()			(this->pdc_peripheral_base_address->PERIPH_PTCR = PERIPH_PTCR_TXTDIS)
#define HAL_PDC_DISABLE_RECEIVER_TRANSFER()				(this->pdc_peripheral_base_address->PERIPH_PTCR = PERIPH_PTCR_RXTDIS)
#define HAL_PDC_READ_RECEIVE_COUNTER_VALUE()			(this->pdc_peripheral_base_address->PERIPH_RCR)





#endif /* HAL_SERIAL_CIRCULAR_BUFFER_H_ */