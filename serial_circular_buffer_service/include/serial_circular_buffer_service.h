/** @file serial_circular_buffer_service.h
 *  @brief class definition for serial circular buffer service
 *  
 *  When instantiated, this class provides the application with the means to transmit
 *  and receive serialized data over a serial UART port.
 *  
 *    
 *  @author Adam Porsch
 *  @bug No known bugs.
 */


#ifndef SERIAL_CIRCULAR_BUFFER_SERVICE_H_
#define SERIAL_CIRCULAR_BUFFER_SERVICE_H_

#include "HAL_serial_circular_buffer.h"
#include "Icomms_circular_buffer.h"

//these enum values correspond with the value required by the microprocessor UART register definitions to configure parity
typedef enum {UART_PARITY_EVEN = 0, UART_PARITY_ODD, UART_PARITY_SPACE, UART_PARITY_MARK, UART_PARITY_NONE} uart_parity_selection_t;
	

	
class serial_circular_buffer : public Icomms_circular_buffer
{
	
		
	public:	
		/**
		 * @brief initialization routine for an instantiated object of type serial_circular_buffer
		 * 
		 * Because the design intent was to statically instantiate instances of this class, a traditional 
		 * constructor was not developed for this class. Therefore, this init function must be called before
		 * the serial_circular_buffer object can be used.
		 * 
		 * @param uart_port_base_addr microprocessor specific peripheral base address of the UART that the circular buffer will use
		 * @param Rx_buffer_ptr pointer to the buffer that will contain the incoming serial bytes
		 * @param Rx_buffer_size_in_bytes size of the incoming serial byte buffer, in bytes
		 * @param Tx_buffer_ptr pointer to the buffer that will contain outgoing serial bytes
		 * @param Tx_buffer_size_in_bytes size of the outgoing serial byte buffer, in bytes
		 * @param baud_rate UART baud rate, in base units of bits/second. Default baud is 115,200
		 * @param parity integer value used by microprocessor UART register to configure parity as defined by uart_parity_selection_t. Default value is no parity.
		 * 
		 * @return void
		 */
		void init(uart_t uart_port_base_addr, 
				  char *Rx_buffer_ptr,
				  uint32_t Rx_buffer_size_in_bytes,
				  char *Tx_buffer_ptr,
				  uint32_t Tx_buffer_size_in_bytes,
				  uint32_t baud_rate = 115200, 
				  uart_parity_selection_t parity = UART_PARITY_NONE);

		/**
		 * @brief returns the latest incoming byte from the circular buffer
		 * 
		 * As bytes arrive over the serial interface, they will automatically be added to the circular buffer.
		 * This function retrieves the next byte in the incoming serial stream to be processed.
		 * The circular buffer head and tail indicies are automatically updated and adjusted when this function is called.		 * 
		 * 
		 * @return char the latest byte from the buffer
		 */
		char		get_latest_byte();
		
		/**
		 * @brief returns the number of unread bytes remaining in the incoming circular buffer
		 * 
		 * Since serial data is likely to be coming in asynchronously, relative to the execution of the application,
		 * this function can be useful when the calling code needs to wait for a certain number of bytes
		 * to arrive before choosing to process them.
		 * 
		 * @return uint32_t the number of unread bytes in the Rx buffer
		 */
		uint32_t	get_number_of_unread_bytes();
		
		/**
		 * @brief copies a formatted serial packet into serial buffer and transmits it (non-blocking)
		 * 
		 * When the consumer of this service has a formatted serial packet residing in memory, this function 
		 * can be called to copy that packet into the next free memory locations in the circular buffer and 
		 * initiate the transmission process.
		 * 
		 * This function is identified as "non-blocking" because it does not wait until every byte has been transmitted
		 * out the serial port before it returns. The function simply notifies the microprocessor that new 
		 * data exists in the circular buffer, kicks off the autonomous transmission process, and returns.
		 * 
		 * @param serialized_data_to_transmit pointer to buffer containing serialized packet to be transmitted
		 * @param number_of_bytes_to_transmit the number of bytes to be transmitted
		 * 
		 * @return void
		 */
		void		copy_packet_into_Tx_buffer_and_transmit(char* serialized_data_to_transmit, uint32_t number_of_bytes_to_transmit);
		
		
		/**
		 * @brief The application should not attempt to call this function
		 * 
		 * In the microprocessor, the USR ISR handlers are fixed, public C functions. In order to integrate these ISR handlers with 
		 * a particular instance of this class, this IRQ handler had to be made public, since the respective ISR handlers 
		 * couldn't call a private member function of this class.
		 * 
		 * @param 
		 * 
		 * @return void
		 */
		void		serial_circular_buffer_irq_handler(void);
	
	
	private:
		/**
		 * @brief calculates the current head index of the incoming circular buffer
		 * 
		 * The microprocessor peripheral DMA controller works by providing its counter a 
		 * fixed number of bytes and a pointer to a buffer in memory. As each serial byte arrives,
		 * the PDC counter is decremented. However, the circular buffer logic requires an incrementing
		 * head index. Therefore, this function calcualtes the head index by returning the difference
		 * between the buffer size and the current couter value.
		 * 
		 * @param 
		 * 
		 * @return uint32_t
		 */
		uint32_t	get_rx_buffer_head_index(void);
		
		void		increment_rx_buffer_tail_index(uint32_t increment_index);
		void		increment_tx_buffer_head_index(uint32_t increment_index);
		void		increment_tx_buffer_tail_index(uint32_t increment_index);
		
		/**
		 * @brief returns the number of bytes that still need to be transmitted
		 * 
		 * When transmitting, PDC peripheral works by the application providing it a number of bytes to transmit
		 * and a pointer to the data to be transfered. The PDC will transmit that data and issue an 
		 * IRQ when its completed. The PDC also requires the data its sending out to reside
		 * in contiguous memory blocks. Therefore, if an outgoing packet is divided between
		 * the latter part and beginning parts of the outgoing circular buffer, this function is called
		 * in the ISR to determine how many more bytes need to be sent in order to complete the
		 * packet transmission.
		 * 
		 * @return uint32_t
		 */
		uint32_t	get_number_of_unsent_bytes();
		void		initiate_PDC_Tx(char *pointer_to_Tx_buffer, uint32_t bytes_to_transfer);		
		
		uart_t		uart_peripheral_base_address;
		pdc_t		pdc_peripheral_base_address;
		char		*rx_buffer;			
		uint32_t	rx_buffer_size;
		char		*pdc_tx_buffer;
		uint32_t	tx_buffer_size;						
		
		uint32_t	tx_buffer_head_index;
		uint32_t	tx_buffer_tail_index;	
			
		//the following variables are declared volatile since they're modified inside an ISR
		volatile uint32_t	rx_buffer_tail_index;
		volatile bool	pdc_Tx_in_progress;		
};


#endif /* SERIAL_CIRCULAR_BUFFER_SERVICE_H_ */