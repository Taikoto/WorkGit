#ifndef ANDROID_HARDWARE_MCU_MCUUARTINTERFACE_PROWAVE_H
#define ANDROID_HARDWARE_MCU_MCUUARTINTERFACE_PROWAVE_H

#include <termios.h>
#include "HardInterface.h"

//#define  MCUUARTPORT "/dev/ttymxc4"
#define  MCUUARTPORT "/dev/ttyMT1"

#define  BAUDRATEINDEXMAX 9
#define  BAUDRATEINDEX    8
#define  GET_BAUDRATEINDEX(index) ( (BAUDRATEINDEXMAX - 1) & index) 
#define TIMEOUT_SEC(buflen, baudrate)    ((buflen)*20/(baudrate) + 2)
#define TIMEOUT_USEC    0



static const int speed_arr[] = {B300, B1200, B2400, B4800, B9600, B19200,
		                                   B38400, B57600, B115200};
enum UARTPROPERTI{ SERIAL_STOPBITS_ONE, SERIAL_STOPBITS_TWO, SERIAL_DATABITS_SEVEN, SERIAL_DATABITS_EIGHT, SERIAL_HARDWARE_UNCTRL, SERIAL_HARDWARE_XONXOFF, \
                                       SERIAL_HARDWARE_CTRL, SERIAL_PARITY_NONE, SERIAL_PARITY_ODD, SERIAL_PARITY_EVEN, SERIAL_PARITY_MARK, SERIAL_PARITY_SPACE };

static const int name_arr[] = {300, 1200, 2400, 4800, 9600, 19200, 38400,
		                               57600, 115200};



class UartInterface: public McuHardWareIo{
		public:
				 UartInterface(MCUIOTYPE iotype):
				 uartiotype(iotype)	
				{
				   init_dev(uartiotype); 
				}

				~UartInterface()
				{
				   uart_close(this->fd); 
				}
                
				int init_dev( MCUIOTYPE uartiotype);
				int txdata(unsigned char *txbuf, unsigned int txcount);
				int rxdata(unsigned char *rxbuf, unsigned int rxcount);
				int uart_set_baudrate(int handle, int bardrateindex);
				int uart_set_properties(int handle, unsigned char databits, unsigned char stopbits, 
								                                     unsigned char parity, unsigned char flow_control);
				int uart_close(int handle);

	   private:
			   MCUIOTYPE uartiotype;
			   bool uart_error;


};
#endif
