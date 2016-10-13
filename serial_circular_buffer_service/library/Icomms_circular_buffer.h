/** @file Icomms_circular_buffer.h
 *  @brief abstract class definition used by serial circular buffer service
 *  
 *  This is the abstract class that the serial_circular_buffer class is derived from.
 *  The higher level code (i.e. LSCP_service) gains access to the circular buffer data via this interface.
 *  
 *  This allows for extension in the future should LSCP library need to transmit and receive data
 *  from another communications interface, such as Ethernet or CAN bus.
 *  
 *  
 *  @author Adam Porsch
 *  @bug No known bugs.
 */


#ifndef ICOMMS_CIRCULAR_BUFFER_H_
#define ICOMMS_CIRCULAR_BUFFER_H_

class Icomms_circular_buffer
{
	public:
		/**
		 * @brief returns the latest incoming byte from the circular buffer
		 * 
		 * @return char the latest byte from the buffer
		 */
		virtual char (get_latest_byte)(void) = 0;
		
		
		/**
		 * @brief returns the number of unread bytes in the incoming circular buffer
		 * 
		 * @param 
		 * 
		 * @return uint32_t the number of unread bytes in the Rx buffer
		 */
		virtual uint32_t (get_number_of_unread_bytes)(void) = 0;
		
		
		/**
		 * @brief copies formatted serial packet into serial buffer and transmits it (non-blocking)
		 * 
		 * @param serialized_data_to_transmit pointer to memory containing formatted packet
		 * @param number_of_bytes_to_transmit the number of bytes that the service will transmit
		 * 
		 * @return void
		 */
		virtual void (copy_packet_into_Tx_buffer_and_transmit)(char* serialized_data_to_transmit, uint32_t number_of_bytes_to_transmit) = 0;
};



#endif /* ICOMMS_CIRCULAR_BUFFER_H_ */